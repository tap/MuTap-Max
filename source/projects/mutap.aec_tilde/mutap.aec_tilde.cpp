/// @file
/// mutap.aec~ — acoustic echo canceller: subtract an adaptive estimate of the
/// loudspeaker→microphone echo path from the microphone signal. The open-loop
/// cousin of mutap.afc~: here a CLEAN far-end reference exists (the signal
/// the patch sends to the speaker), so nothing the canceller does feeds back
/// around — what survives from the feedback problem is DOUBLE-TALK, the
/// near-end talker speaking over the echo. Wraps the same
/// tap::mu::pem_afc<double> engines as mutap.afc~ (the paper the PEM structure
/// implements — Gil-Cacho et al. 2014 — is an open-loop double-talk-robust
/// AEC framework before MuTap borrowed it for feedback): the near-end model
/// is re-fit every block and whitened out of the adaptive update, which is
/// what lets adaptation keep running through double-talk without a
/// double-talk detector. @warp swaps the speech-cascade near-end model for
/// the frequency-warped one built for music/tonal material (it won the
/// measured music double-talk suppression in every room tried — see
/// tests/test_aec.cpp in MuTap). @kalman swaps the NLMS core for the
/// frequency-domain Kalman filter (v2): @mu is ignored, @gate selects the
/// burst floor — and it is the measured double-talk winner (the near-end PSD
/// it tracks per bin is exactly a double-talk model).
///
/// Signal inlet 0 is the microphone signal y (echo + near end); signal
/// inlet 1 is the far-end reference x — the signal the patch sends to the
/// loudspeaker, tapped where it actually reaches the speaker. Signal
/// outlet 0 is the cleaned signal e = y - F_hat x (the near end with the
/// echo removed); the rightmost outlet reports the IPC double-talk
/// indicator (0..1, low = near-end speech dominates, high = unmodeled echo
/// dominates) every few processed blocks.
///
/// @postfilter swaps the whole engine for the measured AEC CHAIN
/// (tap::mu::aec_chain): the RAW frequency-domain Kalman canceller — not PEM;
/// open-loop AEC has an exogenous far end, so the predictor refit buys
/// nothing and measurably floors the misalignment — plus the
/// coherence-driven residual suppressor, comfort noise matched to the
/// near-end noise floor, and the initial receive guard. This is the
/// configuration MuTap's ITU-T compliance battery certifies at 48 and
/// 16 kHz (docs/itu-compliance.md in MuTap; every requirement met at both
/// rates), built from the library's own preset (tap::mu::aec_chain_preset),
/// which rescales every per-block time constant for the actual @block and
/// host sample rate. With @postfilter on, @mu, @warp and @kalman are ignored (the
/// chain's canceller is already the Kalman core) and @gate selects the
/// initial receive guard instead of the NLMS gating stack; the right
/// outlet reports the suppressor's echo-explained fraction instead of IPC.
///
/// The canceller works on fixed blocks of @block samples, independent of the
/// host signal vector size: the perform routine gathers samples into
/// constructor-allocated block buffers, calls process_block() every time a
/// block fills, and plays the processed block back out — adding exactly
/// @block samples of latency on the cleaned output (@postfilter adds one
/// further block: the suppressor's constrained causal gain filter). The
/// perform path is allocation-free (mutap's real-time contract: everything
/// after construction is noexcept and allocation-free).
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
#include <variant>
#include <vector>

#include "c74_min.h"
#include "mutap/fd_kalman.h"
#include "mutap/nn_chain.h"
#include "mutap/pem_afc.h"
#include "mutap/postfilter.h"
#include "mutap_nn_weights_default.h"

using namespace c74::min;

class mutap_aec : public object<mutap_aec>, public vector_operator<> {
    using speech_aec = tap::mu::pem_afc<double>;
    using warped_aec = tap::mu::pem_afc<double, tap::mu::warped_lpc_predictor<double>>;
    using kalman_speech_aec =
        tap::mu::pem_afc<double, tap::mu::speech_predictor<double>, tap::mu::partitioned_fdkf<double>>;
    using kalman_warped_aec =
        tap::mu::pem_afc<double, tap::mu::warped_lpc_predictor<double>, tap::mu::partitioned_fdkf<double>>;
    using chain_aec    = tap::mu::aec_chain<double>;    ///< raw Kalman canceller + suppressor + comfort noise
    using nn_chain_aec = tap::mu::aec_chain_nn<double>; ///< the same chain with the LEARNED post engine

    /// One canceller plus its vector-size bridging buffers, all sized for one
    /// block. Built on the control thread; used (and only used) on the audio
    /// thread after ownership is handed over. The variant selects the
    /// near-end model (@warp); the audio thread dispatches with std::visit,
    /// which never allocates.
    struct engine {
        std::variant<speech_aec, warped_aec, kalman_speech_aec, kalman_warped_aec, chain_aec, nn_chain_aec> aec;
        std::vector<double> x_block; ///< gathering far-end (reference) samples
        std::vector<double> y_block; ///< gathering microphone samples
        std::vector<double> e_block; ///< last processed block, being played out
        size_t              fill{0}; ///< samples gathered so far, == play-out position

        template <typename Aec>
        explicit engine(std::in_place_type_t<Aec> which, const typename Aec::config& cfg, size_t block)
            : aec(which, cfg)
            , x_block(block, 0.0)
            , y_block(block, 0.0)
            , e_block(block, 0.0) {}
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
    std::mutex  m_control_mutex;
    long        m_filter_length{2048}; ///< requested filter length, samples (creation arg)
    long        m_block_size{256};     ///< requested canceller block size, samples
    bool        m_gate{true};          ///< IPC/transient robustness layer on rebuilds
    bool        m_warp{false};         ///< frequency-warped (music) near-end model on rebuilds
    bool        m_kalman{false};       ///< frequency-domain Kalman core (v2) on rebuilds
    long        m_postfilter{0};       ///< post-filter engine: 0 off, 1 classical chain, 2 learned chain
    std::string m_model_path{};        ///< learned-engine weights file ("" = built-in model)
    bool        m_comfort{true};       ///< comfort-noise fill inside the chain on rebuilds
    double      m_chain_sr{0.0};       ///< sample rate the active chain was scaled for (0 = none built)
    bool        m_constructed{false};  ///< guards publish() until the constructor body ran

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
    MIN_DESCRIPTION{"Acoustic echo canceller. Subtracts an adaptive estimate of the "
                    "loudspeaker-to-microphone echo path from the microphone signal — the open-loop "
                    "cousin of mutap.afc~, for the case where a clean far-end reference exists. "
                    "Adaptation runs on a PEM-prewhitened signal pair, so it survives double-talk "
                    "without a double-talk detector (FDAF-PEM, MuTap pem_afc). Inlet 1 takes the "
                    "microphone, inlet 2 the far-end signal feeding the loudspeaker; the cleaned "
                    "output is delayed by @block samples. The right outlet reports the IPC "
                    "double-talk indicator (0..1). @warp selects the frequency-warped near-end model "
                    "for music/tonal sources; @kalman selects the frequency-domain Kalman engine "
                    "(v2), the measured double-talk winner. @postfilter engages the full measured "
                    "AEC chain — raw Kalman canceller, coherence-driven residual suppressor, comfort "
                    "noise, initial receive guard — the configuration MuTap's ITU-T compliance "
                    "battery certifies."};
    MIN_TAGS{"audio, adaptive, echo, cleaning"};
    MIN_AUTHOR{"MuTap contributors"};
    MIN_RELATED{"mutap.afc~, adc~, dac~, adoutput~"};

    inlet<>  m_in_mic{this, "(signal) microphone signal y (echo + near end)"};
    inlet<>  m_in_ref{this, "(signal) far-end / reference signal x (the signal sent to the loudspeaker)"};
    outlet<> m_out{this, "(signal) cleaned signal e = y - estimated echo", "signal"};
    outlet<thread_check::scheduler, thread_action::fifo> m_ipc_out{this, "(float) IPC double-talk indicator, 0..1"};

    /// First creation argument is the echo-path filter length in samples
    /// (default 2048); partitions = ceil(filter_length / block size).
    explicit mutap_aec(const atoms& args = {}) {
        if (!args.empty()) {
            m_filter_length = std::clamp(static_cast<long>(args[0]), k_min_block, k_max_filter_length);
        }
        std::lock_guard<std::mutex> lock(m_control_mutex);
        m_constructed = true;
        publish();
    }

    ~mutap_aec() {
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
                         setter{MIN_FUNCTION{
                             const long requested = args[0];
                             long       rounded   = k_min_block;
                             while (rounded < requested && rounded < k_max_block) {
                                 rounded *= 2;
                             }
                             std::lock_guard<std::mutex> lock(m_control_mutex);
                             m_block_size = rounded;
                             if (m_constructed) {
                                 publish();
                             }
                             return {static_cast<int>(m_block_size)};
                         }}};

    attribute<number> mu{this, "mu", 0.5,
                         description{"NLMS adaptation step size, clamped to (0, 2). 0.5 is a robust default; "
                                     "smaller adapts slower but tracks more calmly. Applied live (no rebuild)."},
                         setter{MIN_FUNCTION{
                             const double value   = args[0];
                             const double clamped = std::clamp(value, 0.001, 1.99);
                             m_mu.store(clamped, std::memory_order_relaxed);
                             return {clamped};
                         }}};

    attribute<bool> adapt{this, "adapt", true,
                          description{"Enable adaptation. Off freezes the learned echo-path estimate; "
                                      "cancellation keeps running with the frozen filter."},
                          setter{MIN_FUNCTION{
                              const bool value = args[0];
                              m_adapt.store(value, std::memory_order_relaxed);
                              return {value};
                          }}};

    attribute<bool> gate{this, "gate", true,
                         description{"Robustness layer for double-talk on the classic engine: scale the step "
                                     "size by IPC^2 and skip updates on near-end transients (transient freeze "
                                     "ratio 4). Freezing through double-talk is the classical AEC answer; the "
                                     "PEM prewhitening already removes most of the need, and the Kalman engine "
                                     "all of it. Changing it rebuilds the canceller from scratch (the learned "
                                     "filter resets). With @warp on, the IPC step scaling stays on even when "
                                     "@gate is off — the warped model requires it. With @postfilter on, @gate "
                                     "selects the chain's initial receive guard instead (switched send loss "
                                     "until convergence certifies, then latched off permanently)."},
                         setter{MIN_FUNCTION{
                             const bool                  value = args[0];
                             std::lock_guard<std::mutex> lock(m_control_mutex);
                             m_gate = value;
                             if (m_constructed) {
                                 publish();
                             }
                             return {value};
                         }}};

    attribute<bool> warp{this, "warp", false,
                         description{"Near-end model: off = the speech cascade (short-term LP + pitch tap), on = the "
                                     "frequency-warped LP built for music/tonal sources — sustained low chords whose "
                                     "packed bass partials defeat the speech model. On music-material double-talk it "
                                     "measured the better echo suppression in every room tried (MuTap test_aec.cpp). "
                                     "Warp keeps IPC step scaling on regardless of @gate (the warped whitener "
                                     "requires it); with the Kalman engine no pairing is needed. Changing it "
                                     "rebuilds the canceller from scratch (the learned filter resets)."},
                         setter{MIN_FUNCTION{
                             const bool                  value = args[0];
                             std::lock_guard<std::mutex> lock(m_control_mutex);
                             m_warp = value;
                             if (m_constructed) {
                                 publish();
                             }
                             return {value};
                         }}};

    attribute<bool> kalman{this, "kalman", false,
                           description{"Adaptive engine: off = the classic NLMS update (mu + gate), on = the "
                                       "frequency-domain Kalman filter (v2) -- per-frequency state uncertainty and "
                                       "near-end tracking replace the step size, so @mu is ignored and @gate selects "
                                       "the burst floor instead. For echo cancellation this is the measured "
                                       "double-talk winner: the near-end PSD it tracks per bin is exactly a "
                                       "double-talk model, no detector needed. The IPC outlet reports 0 with the "
                                       "Kalman engine (its gating is internal). Changing it rebuilds the canceller "
                                       "from scratch (the learned filter resets)."},
                           setter{MIN_FUNCTION{
                               const bool                  value = args[0];
                               std::lock_guard<std::mutex> lock(m_control_mutex);
                               m_kalman = value;
                               if (m_constructed) {
                                   publish();
                               }
                               return {value};
                           }}};

    attribute<int> postfilter{this, "postfilter", 0,
                              description{"Residual-echo post-filter engine: 0 = off (the bare adaptive canceller "
                                          "selected by the attributes above), 1 = the CLASSICAL chain — the raw "
                                          "frequency-domain Kalman canceller plus the coherence-driven residual "
                                          "suppressor, comfort noise matched to the near-end noise floor, and the "
                                          "initial receive guard; the configuration MuTap's ITU-T compliance battery "
                                          "certifies at 48 and 16 kHz (single-talk residual below -76 dBm0(A), "
                                          "double-talk near-end cost about 1 dB, full-duplex P.340 Category 1), with "
                                          "its time constants rescaled for the actual block size and sample rate. "
                                          "2 = the LEARNED chain: the same canceller and guard with the post-filter "
                                          "replaced by a small trained network predicting per-band gains (see @model) "
                                          "— measured stronger single-talk echo removal on speech-like material at "
                                          "equal near-end transparency, weaker double-talk suppression on material "
                                          "unlike its training data; the classical engine remains the certified "
                                          "default. With postfilter nonzero, @mu, @warp and @kalman are ignored (the "
                                          "chain's canceller is already the Kalman core; PEM buys nothing open-loop) "
                                          "and @gate selects the initial receive guard; the right outlet reports the "
                                          "post-filter's echo-explained fraction (0..1) instead of IPC. The learned "
                                          "engine requires @block to equal its model's trained block size (256 for "
                                          "the built-in model) and coerces it, with a console notice, if it does "
                                          "not. Adds one extra block of latency. Old patches with postfilter 0/1 "
                                          "keep their exact meaning. Changing it rebuilds the canceller from "
                                          "scratch (the learned filter resets)."},
                              setter{MIN_FUNCTION{
                                  const long value = std::clamp<long>(static_cast<long>(args[0]), 0, 2);
                                  std::lock_guard<std::mutex> lock(m_control_mutex);
                                  m_postfilter = value;
                                  if (m_constructed) {
                                      publish();
                                  }
                                  return {static_cast<int>(m_postfilter)};
                              }}};

    attribute<symbol> model{this, "model", "",
                            description{"Learned-engine weights (postfilter 2 only): a path to a trained MUNN "
                                        "model file (MuTap's tools/ml pipeline exports these), or empty for the "
                                        "package's built-in 48 kHz model. A model carries its own analysis "
                                        "geometry; @block must equal its trained block size and is coerced, with "
                                        "a console notice, if it does not. A file that cannot be read or "
                                        "validated posts an error and leaves the running engine unchanged. "
                                        "Changing it rebuilds the canceller from scratch (the learned filter "
                                        "resets)."},
                            setter{MIN_FUNCTION{
                                const symbol                value = args[0];
                                std::lock_guard<std::mutex> lock(m_control_mutex);
                                m_model_path = std::string(value);
                                if (m_constructed && m_postfilter == 2) {
                                    publish();
                                }
                                return {value};
                            }}};

    attribute<bool> comfort{
        this, "comfort", true,
        description{"Comfort noise (postfilter only): fill suppressed bins with noise matched to the "
                    "tracked near-end noise floor (two-window minimum statistics), so the far end "
                    "does not hear the room breathe as suppression comes and goes. Fill only — "
                    "never subtraction. Off leaves suppressed bins silent (G.168's Figure 9 "
                    "measurement convention). Changing it rebuilds the canceller from scratch "
                    "(the learned filter resets)."},
        setter{MIN_FUNCTION{
            const bool                  value = args[0];
            std::lock_guard<std::mutex> lock(m_control_mutex);
            m_comfort = value;
            if (m_constructed) {
                publish();
            }
            return {value};
        }}};

    /// Zero the learned filter and the block buffers (applied on the audio
    /// thread at the next vector, so it does not race the perform routine).
    message<> reset{this, "reset", "Reset the canceller: zero the learned echo-path estimate.",
                    MIN_FUNCTION{
                        m_reset_request.store(true, std::memory_order_relaxed);
                        return {};
                    }};

    /// The chain's time constants are scaled for the host sample rate, so a
    /// rate change while @postfilter is on rebuilds the chain (the bare
    /// engines are rate-agnostic and keep running).
    message<> dspsetup{this, "dspsetup",
                       MIN_FUNCTION{
                           std::lock_guard<std::mutex> lock(m_control_mutex);
                           if (m_constructed && m_postfilter != 0 && static_cast<double>(args[0]) != m_chain_sr) {
                               publish();
                           }
                           return {};
                       }};

    void operator()(audio_bundle input, audio_bundle output) {
        const auto    frames = input.frame_count();
        const double* y_in   = input.samples(0);
        const double* x_in   = input.samples(1);
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
        std::visit(
            [&](auto& aec) {
                aec.set_adaptation(m_adapt.load(std::memory_order_relaxed));
                if constexpr (requires { aec.fdaf().set_step_size(0.0); }) {
                    aec.fdaf().set_step_size(m_mu.load(std::memory_order_relaxed));
                }
                if (m_reset_request.exchange(false, std::memory_order_relaxed)) {
                    aec.reset();
                    std::fill(eng.x_block.begin(), eng.x_block.end(), 0.0);
                    std::fill(eng.y_block.begin(), eng.y_block.end(), 0.0);
                    std::fill(eng.e_block.begin(), eng.e_block.end(), 0.0);
                    eng.fill = 0;
                }

                // Vector-size bridging: gather into the block buffers, process
                // every time a block fills, play the processed block back out —
                // exactly block_size samples of latency, for host vectors smaller
                // or larger than the block. Inputs are read before the output is
                // written because Max may alias output buffers onto input buffers.
                const size_t b = aec.block_size();
                for (auto i = 0; i < frames; ++i) {
                    const double x        = x_in[i];
                    const double y        = y_in[i];
                    out[i]                = eng.e_block[eng.fill];
                    eng.x_block[eng.fill] = x;
                    eng.y_block[eng.fill] = y;
                    if (++eng.fill == b) {
                        aec.process_block(eng.x_block.data(), eng.y_block.data(), eng.e_block.data());
                        eng.fill = 0;
                        if (--m_report_countdown <= 0) {
                            m_report_countdown = k_ipc_report_blocks;
                            if constexpr (requires { aec.ipc(); }) {
                                m_ipc_atoms[0] = aec.ipc();
                            }
                            else { // the chain: echo-explained fraction (0..1, high = converged on echo)
                                m_ipc_atoms[0] = aec.postfilter().echo_explained();
                            }
                            m_ipc_out.send(m_ipc_atoms);
                        }
                    }
                }
            },
            eng.aec);
    }

  private:
    /// Assemble a pem_afc config from the current control-side state; the two
    /// instantiations share every field this external sets (the predictor
    /// configs differ, but both have analysis_capacity). The clamping in the
    /// setters keeps every constraint satisfied, so the canceller constructor
    /// does not throw for any reachable combination.
    template <typename Aec>
    typename Aec::config make_config() const {
        typename Aec::config cfg;
        const auto           b          = static_cast<size_t>(m_block_size);
        cfg.fdaf.block_size             = b;
        cfg.fdaf.partitions             = std::max<size_t>(1, (static_cast<size_t>(m_filter_length) + b - 1) / b);
        cfg.fdaf.step_size              = m_mu.load(std::memory_order_relaxed);
        cfg.fdaf.ipc_step_scaling       = m_gate || m_warp; // the warped whitener requires the IPC scale
        cfg.fdaf.transient_freeze_ratio = m_gate ? 4.0 : 0.0;
        // analysis_window must be a multiple of block_size and >= 2 * block_size;
        // both operands are powers of two, so the max is always a multiple.
        cfg.analysis_window             = std::max<size_t>(2 * b, 1024);
        cfg.predictor.analysis_capacity = std::max(cfg.predictor.analysis_capacity, cfg.analysis_window);
        return cfg;
    }

    /// Same, for the Kalman-core instantiations: no step size and no IPC
    /// options exist; @gate maps to the opt-in transient (burst) floor.
    template <typename Aec>
    typename Aec::config make_kalman_config() const {
        typename Aec::config cfg;
        const auto           b          = static_cast<size_t>(m_block_size);
        cfg.fdaf.block_size             = b;
        cfg.fdaf.partitions             = std::max<size_t>(1, (static_cast<size_t>(m_filter_length) + b - 1) / b);
        cfg.fdaf.transient_floor_ratio  = m_gate ? 8.0 : 0.0;
        cfg.analysis_window             = std::max<size_t>(2 * b, 1024);
        cfg.predictor.analysis_capacity = std::max(cfg.predictor.analysis_capacity, cfg.analysis_window);
        return cfg;
    }

    /// The compliance chain IS the library preset (tap::mu::aec_chain_preset —
    /// the configuration MuTap's ITU battery certifies, with every per-block
    /// time constant rescaled for the actual block and sample rate; its
    /// header documents the rule and the two deliberate exceptions). Only the
    /// external's own toggles are applied on top.
    typename chain_aec::config make_chain_config(double sr) const {
        const auto b   = static_cast<size_t>(m_block_size);
        auto       cfg = tap::mu::aec_chain_preset<double>(
            b, std::max<size_t>(1, (static_cast<size_t>(m_filter_length) + b - 1) / b), sr);
        cfg.postfilter.comfort_noise = m_comfort;
        // @gate selects the initial receive guard (switched < 14 dB send loss
        // until convergence certifies, latched off permanently — the
        // convergence-in-noise clause is unmeetable without it).
        if (!m_gate) {
            cfg.guard_attenuation_db = 0.0;
        }
        return cfg;
    }

    /// The learned-engine chain: same canceller/guard calibration as the
    /// classical preset (tap::mu::aec_chain_nn_preset), post-filter swapped
    /// for the trained model — @model's file, or the built-in weights. The
    /// model's trained block size is authoritative: publish() coerces
    /// @block to it (with a console notice) before construction.
    typename nn_chain_aec::config make_nn_chain_config(double sr, tap::mu::nn_suppressor_weights weights) const {
        const auto b   = static_cast<size_t>(m_block_size);
        auto       cfg = tap::mu::aec_chain_nn_preset<double>(
            b, std::max<size_t>(1, (static_cast<size_t>(m_filter_length) + b - 1) / b), sr, std::move(weights));
        cfg.postfilter.comfort_noise = m_comfort;
        if (!m_gate) {
            cfg.guard_attenuation_db = 0.0;
        }
        return cfg;
    }

    /// Build a canceller from the current config and publish it for the audio
    /// thread to adopt. Caller holds m_control_mutex.
    void publish() {
        delete m_trash.exchange(nullptr, std::memory_order_acq_rel); // reap
        try {
            const auto              b = static_cast<size_t>(m_block_size);
            std::unique_ptr<engine> eng;
            if (m_postfilter == 2) {
                const double sr      = samplerate() > 0.0 ? samplerate() : 48000.0;
                auto         weights = m_model_path.empty() ? tap::mu::parse_nn_suppressor_weights(k_nn_weights_default,
                                                                                                   sizeof(k_nn_weights_default))
                                                            : tap::mu::load_nn_suppressor_weights(m_model_path.c_str());
                if (weights.geometry.hop != b) {
                    cout << "postfilter 2: block coerced to the model's trained block size ("
                         << static_cast<int>(weights.geometry.hop) << ")" << endl;
                    m_block_size = static_cast<long>(weights.geometry.hop);
                }
                if (weights.geometry.sample_rate != sr) {
                    cout << "postfilter 2: model trained at " << weights.geometry.sample_rate << " Hz, running at "
                         << sr << " Hz (detuned bands; train a matching model for " << "best results)" << endl;
                }
                const auto bn = static_cast<size_t>(m_block_size);
                eng           = std::make_unique<engine>(std::in_place_type<nn_chain_aec>,
                                                         make_nn_chain_config(sr, std::move(weights)), bn);
                m_chain_sr    = sr;
            }
            else if (m_postfilter == 1) {
                const double sr = samplerate() > 0.0 ? samplerate() : 48000.0;
                eng             = std::make_unique<engine>(std::in_place_type<chain_aec>, make_chain_config(sr), b);
                m_chain_sr      = sr;
            }
            else if (m_kalman) {
                eng = m_warp ? std::make_unique<engine>(std::in_place_type<kalman_warped_aec>,
                                                        make_kalman_config<kalman_warped_aec>(), b)
                             : std::make_unique<engine>(std::in_place_type<kalman_speech_aec>,
                                                        make_kalman_config<kalman_speech_aec>(), b);
            }
            else {
                eng = m_warp ? std::make_unique<engine>(std::in_place_type<warped_aec>, make_config<warped_aec>(), b)
                             : std::make_unique<engine>(std::in_place_type<speech_aec>, make_config<speech_aec>(), b);
            }
            // A still-unadopted previous pending engine comes back to us here
            // and is deleted — the audio thread only ever sees the newest one.
            delete m_pending.exchange(eng.release(), std::memory_order_acq_rel);
        }
        catch (const std::exception& ex) {
            // Leave the current canceller running (the perform path falls back
            // to the dry microphone signal if none exists yet) and say why —
            // a bad @model file lands here.
            cerr << "engine rebuild failed: " << ex.what() << endl;
        }
    }
};

MIN_EXTERNAL(mutap_aec);
