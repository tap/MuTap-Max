/// @file
/// mutap.defeed~ — acoustic feedback (howling) canceller: subtract an adaptive
/// estimate of the loudspeaker→microphone feedback path from the microphone
/// signal. Wraps mutap::pem_afc<double> (FDAF-PEM-AFROW: a partitioned-block
/// frequency-domain adaptive filter whose update is decorrelated from the
/// near-end source by prediction-error-method prewhitening — the closed loop
/// biases any naive adaptive estimate, and the PEM prewhitening removes that
/// bias; Gil-Cacho et al. 2014, Rombouts et al. 2007).
///
/// Signal inlet 0 is the microphone signal y; signal inlet 1 is the
/// loudspeaker/reference signal u (the signal the patch sends to the
/// speaker). Signal outlet 0 is the cleaned signal e = y - F_hat u; the
/// rightmost outlet reports the IPC double-talk indicator (0..1, low =
/// near-end speech dominates, high = feedback dominates) every few processed
/// blocks.
///
/// The canceller works on fixed blocks of @block samples, independent of the
/// host signal vector size: the perform routine gathers samples into
/// constructor-allocated block buffers, calls process_block() every time a
/// block fills, and plays the processed block back out — adding exactly
/// @block samples of latency on the cleaned output. The perform path is
/// allocation-free (mutap's real-time contract: everything after
/// construction is noexcept and allocation-free).
///
/// Threading follows the ambitap.xtc~ pattern: attribute setters run on the
/// control thread, where structural changes (@block, @gate, the filter-length
/// creation arg) rebuild the canceller and publish it through a lock-free
/// single-slot handoff; the audio thread adopts the new canceller at a vector
/// boundary and parks the old one in a trash slot that the control thread
/// reaps — the audio thread never allocates or frees. Scalar controls (@mu,
/// @adapt, reset) travel through atomics and are applied on the audio thread.
// SPDX-License-Identifier: MIT
// Copyright 2026 MuTap contributors

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

#include "c74_min.h"
#include "mutap/pem_afc.h"

using namespace c74::min;

class mutap_defeed : public object<mutap_defeed>, public vector_operator<> {
    /// One canceller plus its vector-size bridging buffers, all sized for one
    /// block. Built on the control thread; used (and only used) on the audio
    /// thread after ownership is handed over.
    struct engine {
        mutap::pem_afc<double> afc;
        std::vector<double>    u_block; ///< gathering reference (loudspeaker) samples
        std::vector<double>    y_block; ///< gathering microphone samples
        std::vector<double>    e_block; ///< last processed block, being played out
        size_t                 fill{0}; ///< samples gathered so far, == play-out position

        explicit engine(const mutap::pem_afc<double>::config& cfg)
            : afc(cfg)
            , u_block(cfg.fdaf.block_size, 0.0)
            , y_block(cfg.fdaf.block_size, 0.0)
            , e_block(cfg.fdaf.block_size, 0.0) {}
    };

    static constexpr long k_min_block         = 16;
    static constexpr long k_max_block         = 4096;
    static constexpr long k_max_filter_length = 65536;
    static constexpr long k_ipc_report_blocks = 8; ///< IPC report every this many processed blocks

    // State lives ABOVE the attributes on purpose: min-api attribute
    // construction invokes the custom setter with the default value, and
    // members are initialized in declaration order — everything a setter
    // touches must already be alive.

    // Control-side state (attribute setters may arrive on both the main and
    // the scheduler thread; the mutex serializes them — the audio thread
    // never takes it).
    std::mutex m_control_mutex;
    long       m_filter_length{2048}; ///< requested filter length, samples (creation arg)
    long       m_block_size{256};     ///< requested canceller block size, samples
    bool       m_gate{true};          ///< IPC/transient robustness layer on rebuilds
    bool       m_constructed{false};  ///< guards publish() until the constructor body ran

    // Scalar controls applied by the audio thread every vector (no rebuild).
    std::atomic<double> m_mu{0.5};
    std::atomic<bool>   m_adapt{true};
    std::atomic<bool>   m_reset_request{false};

    // Control -> audio handoff (freshly built engine awaiting adoption) and
    // audio -> control return path (retired engine awaiting deletion).
    std::atomic<engine*> m_pending{nullptr};
    std::atomic<engine*> m_trash{nullptr};

    // Audio-thread-only state. m_ipc_atoms is pre-allocated because the
    // queue-backed outlet's scalar send() does not compile in this min-api
    // pin (its unsafe-send path pushes the raw scalar where atoms& is
    // expected); sending an atoms lvalue takes the path that does. The
    // queue copy inside min-api is the same cost min.edge~ pays.
    engine* m_active{nullptr};
    long    m_report_countdown{k_ipc_report_blocks};
    atoms   m_ipc_atoms{0.0};

  public:
    MIN_DESCRIPTION{"Acoustic feedback (howling) canceller. Subtracts an adaptive estimate of the "
                    "loudspeaker-to-microphone feedback path from the microphone signal, adapted "
                    "with PEM prewhitening so the closed loop does not bias the estimate "
                    "(FDAF-PEM-AFROW, MuTap pem_afc). Inlet 1 takes the microphone, inlet 2 the "
                    "signal feeding the loudspeaker; the cleaned output is delayed by @block samples. "
                    "The right outlet reports the IPC double-talk indicator (0..1)."};
    MIN_TAGS{"audio, adaptive, feedback, howling, cleaning"};
    MIN_AUTHOR{"MuTap contributors"};
    MIN_RELATED{"adc~, dac~, adoutput~"};

    inlet<>  m_in_mic{this, "(signal) microphone signal y"};
    inlet<>  m_in_ref{this, "(signal) loudspeaker / reference signal u"};
    outlet<> m_out{this, "(signal) cleaned signal e = y - estimated feedback", "signal"};
    outlet<thread_check::scheduler, thread_action::fifo> m_ipc_out{this, "(float) IPC double-talk indicator, 0..1"};

    /// First creation argument is the feedback-path filter length in samples
    /// (default 2048); partitions = ceil(filter_length / block size).
    explicit mutap_defeed(const atoms& args = {}) {
        if (!args.empty()) {
            m_filter_length = std::clamp(static_cast<long>(args[0]), k_min_block, k_max_filter_length);
        }
        std::lock_guard<std::mutex> lock(m_control_mutex);
        m_constructed = true;
        publish();
    }

    ~mutap_defeed() {
        // DSP is torn down before the object is freed; every live engine is
        // in exactly one of these three places.
        delete m_pending.exchange(nullptr);
        delete m_trash.exchange(nullptr);
        delete m_active;
    }

    attribute<int> block{this, "block", 256,
                         description{"Canceller block size in samples (rounded up to a power of 2, 16-4096). "
                                     "Sets the adaptation hop, the added output latency, and — with the "
                                     "filter-length creation arg — the partition count. Changing it rebuilds "
                                     "the canceller from scratch (the learned filter resets)."},
                         setter{MIN_FUNCTION{const long requested = args[0];
    long rounded = k_min_block;
    while (rounded < requested && rounded < k_max_block) {
        rounded *= 2;
    }
    std::lock_guard<std::mutex> lock(m_control_mutex);
    m_block_size = rounded;
    if (m_constructed) {
        publish();
    }
    return {static_cast<int>(m_block_size)};
}
}
}
;

attribute<number> mu{this, "mu", 0.5,
                     description{"NLMS adaptation step size, clamped to (0, 2). 0.5 is a robust default; "
                                 "smaller adapts slower but tracks more calmly. Applied live (no rebuild)."},
                     setter{MIN_FUNCTION{const double value = args[0];
const double clamped = std::clamp(value, 0.001, 1.99);
m_mu.store(clamped, std::memory_order_relaxed);
return {clamped};
}
}
}
;

attribute<bool> adapt{this, "adapt", true,
                      description{"Enable adaptation. Off freezes the learned feedback-path estimate; "
                                  "cancellation keeps running with the frozen filter."},
                      setter{MIN_FUNCTION{const bool value = args[0];
m_adapt.store(value, std::memory_order_relaxed);
return {value};
}
}
}
;

attribute<bool>             gate{this, "gate", true,
                     description{"Robustness layer for double-talk: scale the step size by IPC^2 and skip "
                                             "updates on near-end transients (transient freeze ratio 4). Changing it "
                                             "rebuilds the canceller from scratch (the learned filter resets)."},
                     setter{MIN_FUNCTION{const bool value = args[0];
std::lock_guard<std::mutex> lock(m_control_mutex);
m_gate = value;
if (m_constructed) {
    publish();
}
return {value};
}
}
}
;

/// Zero the learned filter and the block buffers (applied on the audio
/// thread at the next vector, so it does not race the perform routine).
message<> reset{this, "reset", "Reset the canceller: zero the learned feedback-path estimate.",
                MIN_FUNCTION{m_reset_request.store(true, std::memory_order_relaxed);
return {};
}
}
;

void operator()(audio_bundle input, audio_bundle output) {
    const auto    frames = input.frame_count();
    const double* y_in   = input.samples(0);
    const double* u_in   = input.samples(1);
    double*       out    = output.samples(0);

    // Adopt a newly published canceller, but only when the trash slot is
    // free to receive the engine we would retire (the control thread reaps
    // it; the audio thread never frees). A full slot just defers the
    // switch to a later vector.
    if (m_trash.load(std::memory_order_relaxed) == nullptr) {
        engine* incoming = m_pending.exchange(nullptr, std::memory_order_acq_rel);
        if (incoming) {
            if (m_active) {
                m_trash.store(m_active, std::memory_order_release);
            }
            m_active = incoming;
        }
    }

    // No canceller (a rebuild failed, or none was ever built): pass the
    // dry microphone signal through.
    if (!m_active) {
        for (auto i = 0; i < frames; ++i) {
            out[i] = y_in[i];
        }
        return;
    }

    engine& eng = *m_active;
    eng.afc.set_adaptation(m_adapt.load(std::memory_order_relaxed));
    eng.afc.fdaf().set_step_size(m_mu.load(std::memory_order_relaxed));
    if (m_reset_request.exchange(false, std::memory_order_relaxed)) {
        eng.afc.reset();
        std::fill(eng.u_block.begin(), eng.u_block.end(), 0.0);
        std::fill(eng.y_block.begin(), eng.y_block.end(), 0.0);
        std::fill(eng.e_block.begin(), eng.e_block.end(), 0.0);
        eng.fill = 0;
    }

    // Vector-size bridging: gather into the block buffers, process every
    // time a block fills, play the processed block back out — exactly
    // block_size samples of latency, for host vectors smaller or larger
    // than the block. Inputs are read before the output is written because
    // Max may alias output buffers onto input buffers.
    const size_t b = eng.afc.block_size();
    for (auto i = 0; i < frames; ++i) {
        const double u        = u_in[i];
        const double y        = y_in[i];
        out[i]                = eng.e_block[eng.fill];
        eng.u_block[eng.fill] = u;
        eng.y_block[eng.fill] = y;
        if (++eng.fill == b) {
            eng.afc.process_block(eng.u_block.data(), eng.y_block.data(), eng.e_block.data());
            eng.fill = 0;
            if (--m_report_countdown <= 0) {
                m_report_countdown = k_ipc_report_blocks;
                m_ipc_atoms[0]     = eng.afc.ipc();
                m_ipc_out.send(m_ipc_atoms);
            }
        }
    }
}

private:
/// Assemble the pem_afc config from the current control-side state. The
/// clamping in the setters keeps every constraint satisfied, so the
/// canceller constructor does not throw for any reachable combination.
mutap::pem_afc<double>::config make_config() const {
    mutap::pem_afc<double>::config cfg;
    const auto                     b = static_cast<size_t>(m_block_size);
    cfg.fdaf.block_size              = b;
    cfg.fdaf.partitions              = std::max<size_t>(1, (static_cast<size_t>(m_filter_length) + b - 1) / b);
    cfg.fdaf.step_size               = m_mu.load(std::memory_order_relaxed);
    cfg.fdaf.ipc_step_scaling        = m_gate;
    cfg.fdaf.transient_freeze_ratio  = m_gate ? 4.0 : 0.0;
    // analysis_window must be a multiple of block_size and >= 2 * block_size;
    // both operands are powers of two, so the max is always a multiple.
    cfg.analysis_window             = std::max<size_t>(2 * b, 1024);
    cfg.predictor.analysis_capacity = std::max(cfg.predictor.analysis_capacity, cfg.analysis_window);
    return cfg;
}

/// Build a canceller from the current config and publish it for the audio
/// thread to adopt. Caller holds m_control_mutex.
void publish() {
    delete m_trash.exchange(nullptr, std::memory_order_acq_rel); // reap
    try {
        auto eng = std::make_unique<engine>(make_config());
        // A still-unadopted previous pending engine comes back to us here
        // and is deleted — the audio thread only ever sees the newest one.
        delete m_pending.exchange(eng.release(), std::memory_order_acq_rel);
    }
    catch (const std::exception&) {
        // Defensive: leave the current canceller running (the perform path
        // falls back to the dry microphone signal if none exists yet).
    }
}
}
;

MIN_EXTERNAL(mutap_defeed);
