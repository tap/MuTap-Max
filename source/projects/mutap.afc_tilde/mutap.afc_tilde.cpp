/// @file
/// mutap.afc~ — acoustic feedback (howling) canceller: subtract an adaptive
/// estimate of the loudspeaker→microphone feedback path from the microphone
/// signal. Wraps tap::mu::pem_afc<double> (FDAF-PEM-AFROW: a partitioned-block
/// frequency-domain adaptive filter whose update is decorrelated from the
/// near-end source by prediction-error-method prewhitening — the closed loop
/// biases any naive adaptive estimate, and the PEM prewhitening removes that
/// bias; Gil-Cacho et al. 2014, Rombouts et al. 2007). @warp swaps the
/// speech-cascade near-end model for the frequency-warped one built for
/// music/tonal material (tap::mu::warped_lpc_predictor), and keeps IPC step
/// scaling on while it is active — the warped whitener requires it for
/// room-robust closed-loop stability (see include/mutap/lpc.h in MuTap).
/// @kalman swaps the NLMS core for the frequency-domain Kalman filter
/// (tap::mu::partitioned_fdkf, the v2 engine): @mu is ignored, @gate selects
/// the burst floor, and the warped model needs no IPC pairing there.
///
/// Signal inlet 0 is the microphone signal y; signal inlet 1 is the
/// loudspeaker/reference signal u (the signal the patch sends to the
/// speaker). Signal outlet 0 is the cleaned signal e = y - F_hat u (times the
/// guard's gain with @guard on); signal outlet 1 is the guard's post-reverb
/// (bus-stage) gain, linear (1 with the guard off); outlet 2 reports the IPC
/// double-talk indicator (0..1, low = near-end speech dominates, high =
/// feedback dominates) every few processed blocks; the rightmost outlet
/// reports two raw convergence statistics on the same cadence, as the list
/// `uncertainty_db shadow_ratio_db`: the Kalman core's identification
/// progress (pem_afc::uncertainty_ratio(); 0 dB with the NLMS core, which has
/// none) and the @shadow comparator's residual power ratio
/// (pem_afc::shadow_residual_ratio(); 0 dB with @shadow 0). MuTap's
/// pem_afc.h and fd_kalman.h record what each was measured to catch and
/// miss; without the guard the external applies no threshold — that is the
/// patch's policy.
///
/// @guard 1 adds MuTap's safety layer for this one microphone: a
/// tap::mu::howl_guard<double> (microphones = 1) built with the canceller,
/// fed every processed block with the canceller's residual e and its two
/// statistics, whose per-mic gain is applied to signal outlet 0 — the
/// single-mic equivalent of afc_chain::set_guard() (include/mutap/howl_guard.h
/// and afc_chain.h in MuTap; docs/howl-guard.md has the measurements). It
/// needs both statistics, as afc_chain does: @kalman 1 and @shadow > 0;
/// otherwise the object posts an error and runs unguarded. With the guard on
/// the rightmost list grows to `uncertainty_db shadow_ratio_db guard_state
/// gain_db unprotected strikes`, and a soundcheck (`calibrate`) ends with
/// `calibrate_done d_db a_db` on the same outlet. The guard's policy
/// attributes are copied into a pending slot and applied by the audio thread
/// at the top of the next vector; the main thread never touches a live guard.
///
/// The canceller works on fixed blocks of @block samples, independent of the
/// host signal vector size: the perform routine gathers samples into
/// constructor-allocated block buffers, calls process_block() every time a
/// block fills, and plays the processed block back out — adding exactly
/// @block samples of latency on the cleaned output (the gain outlet is
/// aligned with it). The perform path is allocation-free (mutap's real-time
/// contract: everything after construction is noexcept and allocation-free).
///
/// Threading follows the ambitap.xtc~ pattern: attribute setters run on the
/// control thread, where structural changes (@block, @gate, @guard, the
/// filter-length creation arg) rebuild the canceller (and the guard) and
/// publish it through a lock-free single-slot handoff; the audio thread
/// adopts the new engine at a vector boundary and parks the old one in a
/// trash slot that the control thread reaps — the audio thread never
/// allocates or frees. Scalar controls (@mu, @adapt, reset, the guard's
/// policy, calibrate, clear) travel through atomics or the policy slot and
/// are applied on the audio thread. The shadow comparator's smoothing is a
/// per-block retention, so it is scaled for the block size and the DSP sample
/// rate at every rebuild, and a sample-rate change (dspsetup) rebuilds while
/// @shadow is on (always, with the guard running).
// SPDX-License-Identifier: MIT
// Copyright 2026 MuTap contributors

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <variant>
#include <vector>

#include "c74_min.h"
#include "mutap/fd_kalman.h"
#include "mutap/howl_guard.h"
#include "mutap/pem_afc.h"

using namespace c74::min;

class mutap_afc : public object<mutap_afc>, public vector_operator<> {
    using speech_afc = tap::mu::pem_afc<double>;
    using warped_afc = tap::mu::pem_afc<double, tap::mu::warped_lpc_predictor<double>>;
    using kalman_speech_afc =
        tap::mu::pem_afc<double, tap::mu::speech_predictor<double>, tap::mu::partitioned_fdkf<double>>;
    using kalman_warped_afc =
        tap::mu::pem_afc<double, tap::mu::warped_lpc_predictor<double>, tap::mu::partitioned_fdkf<double>>;
    using guard_type   = tap::mu::howl_guard<double>;
    using guard_policy = tap::mu::guard_policy;

    /// One canceller plus its vector-size bridging buffers, all sized for one
    /// block, and the optional guard (@guard) at the same block size. Built on
    /// the control thread; used (and only used) on the audio thread after
    /// ownership is handed over. The variant selects the near-end model
    /// (@warp); the audio thread dispatches with std::visit, which never
    /// allocates. The guard is never moved (it points into itself), so it is
    /// emplaced in place and the engine lives behind a pointer.
    struct engine {
        std::variant<speech_afc, warped_afc, kalman_speech_afc, kalman_warped_afc> afc;
        std::vector<double>       u_block;            ///< gathering reference (loudspeaker) samples
        std::vector<double>       y_block;            ///< gathering microphone samples
        std::vector<double>       e_block;            ///< last processed block (guarded), being played out
        std::vector<double>       g_block;            ///< its bus-stage gain, being played out (1 without a guard)
        std::optional<guard_type> guard;              ///< the safety layer, when @guard is on and supported
        size_t                    fill{0};            ///< samples gathered so far, == play-out position
        bool                      calibrating{false}; ///< a soundcheck is running (audio thread)

        template <typename Afc>
        explicit engine(std::in_place_type_t<Afc> which, const typename Afc::config& cfg)
            : afc(which, cfg)
            , u_block(cfg.fdaf.block_size, 0.0)
            , y_block(cfg.fdaf.block_size, 0.0)
            , e_block(cfg.fdaf.block_size, 0.0)
            , g_block(cfg.fdaf.block_size, 1.0) {}
    };

    /// The guard's policy on its way to the audio thread, and whether the
    /// verdict thresholds (@d_db, @a_db) changed since the last hand-over.
    struct policy_update {
        guard_policy policy;
        bool         thresholds{false};
    };

    /// calibrate requests, control -> audio.
    enum class calibration_request : int { none, begin, end };

    static constexpr long k_min_block         = 16;
    static constexpr long k_max_block         = 4096;
    static constexpr long k_max_filter_length = 65536;
    static constexpr long k_ipc_report_blocks = 8; ///< IPC report every this many processed blocks
    /// Most partitions any reachable geometry has (longest filter, smallest
    /// block): @shadow's setter bound. The build clamps to the actual count.
    static constexpr long k_max_partitions = k_max_filter_length / k_min_block;
    /// Physical time constant of the shadow comparator's residual-power
    /// smoothing: pem_afc's default retention 0.9742 is exp(-64 / (48000 *
    /// 0.051)), so this keeps that 51 ms at any @block and sample rate.
    static constexpr double k_shadow_tau_s = 0.051;
    /// Floor of the linear statistics before the dB conversion (-120 dB).
    static constexpr double k_stat_floor = 1e-12;
    /// The guard's howl detector keeps its band range under this fraction of
    /// the sample rate (its default top band, 16 kHz, must stay below
    /// Nyquist: below 35.6 kHz it is lowered to 0.45 fs).
    static constexpr double k_detector_top = 0.45;

    // State lives ABOVE the attributes on purpose: min-api attribute
    // construction invokes the custom setter with the default value, and
    // members are initialized in declaration order — everything a setter
    // touches must already be alive.

    // Control-side state (attribute setters may arrive on both the main and
    // the scheduler thread; the mutex serializes them — the audio thread
    // never takes it).
    mutable std::mutex m_control_mutex;
    long               m_filter_length{2048}; ///< requested filter length, samples (creation arg)
    long               m_block_size{256};     ///< requested canceller block size, samples
    bool               m_gate{true};          ///< IPC/transient robustness layer on rebuilds
    bool               m_warp{false};         ///< frequency-warped (music) near-end model on rebuilds
    bool               m_kalman{false};       ///< frequency-domain Kalman core (v2) on rebuilds
    long               m_shadow{2};           ///< requested shadow-comparator partitions (0 = off) on rebuilds
    double             m_engine_sr{0.0};      ///< sample rate the last built engine was scaled for (0 = none)
    size_t             m_engine_shadow{0};    ///< shadow partitions the last built engine was configured with
    bool               m_guard{false};        ///< @guard requested (it runs only with @kalman 1 and @shadow > 0)
    bool               m_engine_guard{false}; ///< the last built engine carries a guard
    bool               m_dsp_seen{false};     ///< dspsetup ran: refusals of @guard are posted from here on
    guard_policy       m_policy;              ///< the guard's policy as the attributes set it
    bool               m_constructed{false};  ///< guards publish() until the constructor body ran

    // Scalar controls applied by the audio thread every vector (no rebuild).
    std::atomic<double>              m_mu{0.5};
    std::atomic<bool>                m_adapt{true};
    std::atomic<bool>                m_reset_request{false};
    std::atomic<bool>                m_clear_request{false};
    std::atomic<calibration_request> m_calibration_request{calibration_request::none};

    // The guard's policy, control -> audio: the control thread writes the
    // slot under the spin flag and raises m_policy_dirty; the audio thread
    // only TRIES the flag (a busy flag defers the update to the next vector),
    // so it never waits on the control thread.
    policy_update     m_policy_slot;
    std::atomic<bool> m_policy_dirty{false};
    std::atomic_flag  m_policy_busy = ATOMIC_FLAG_INIT;

    // Control -> audio handoff (freshly built engine awaiting adoption) and
    // audio -> control return path (retired engine awaiting deletion).
    std::atomic<engine*> m_pending{nullptr};
    std::atomic<engine*> m_trash{nullptr};

    // Audio-thread-only state. The atoms are pre-allocated because the
    // queue-backed outlet's scalar send() does not compile in this min-api
    // pin (its unsafe-send path pushes the raw scalar where atoms& is
    // expected); sending an atoms lvalue takes the path that does. The queue
    // copy inside min-api is the same cost min.edge~ pays. The symbols are
    // made here, on the constructing thread, so the audio thread only copies
    // pointers.
    engine*               m_active{nullptr};
    long                  m_report_countdown{k_ipc_report_blocks};
    atoms                 m_ipc_atoms{0.0};
    atoms                 m_conv_atoms{0.0, 0.0}; ///< uncertainty_db, shadow_ratio_db
    std::array<symbol, 6> m_state_names{symbol{"arming"}, symbol{"open"},      symbol{"open_capped"},
                                        symbol{"ducked"}, symbol{"releasing"}, symbol{"latched"}};
    /// uncertainty_db shadow_ratio_db guard_state gain_db unprotected strikes
    atoms m_guard_atoms{0.0, 0.0, symbol{"arming"}, 0.0, 0, 0};
    atoms m_calibrated_atoms{symbol{"calibrate_done"}, 0.0, 0.0}; ///< calibrate_done d_db a_db

  public:
    MIN_DESCRIPTION{"Acoustic feedback (howling) canceller. Subtracts an adaptive estimate of the "
                    "loudspeaker-to-microphone feedback path from the microphone signal, adapted "
                    "with PEM prewhitening so the closed loop does not bias the estimate "
                    "(FDAF-PEM-AFROW, MuTap pem_afc). Inlet 1 takes the microphone, inlet 2 the "
                    "signal feeding the loudspeaker; the cleaned output is delayed by @block samples. "
                    "The second outlet carries the guard's post-reverb gain (1 with the guard off); the "
                    "third reports the IPC double-talk indicator (0..1); the right outlet reports two raw "
                    "convergence statistics in dB (uncertainty_db shadow_ratio_db), plus the guard's "
                    "state with @guard on. @warp selects the frequency-warped near-end model for "
                    "music/tonal sources; @kalman selects the frequency-domain Kalman engine (v2); "
                    "@shadow sizes the shadow comparator; @guard adds the anti-howl safety layer "
                    "(MuTap howl_guard: arming, duck, re-arm, back-off, soundcheck calibration, cap)."};
    MIN_TAGS{"audio, adaptive, feedback, howling, cleaning"};
    MIN_AUTHOR{"MuTap contributors"};
    MIN_RELATED{"adc~, dac~, adoutput~"};

    inlet<>  m_in_mic{this, "(signal) microphone signal y"};
    inlet<>  m_in_ref{this, "(signal) loudspeaker / reference signal u"};
    outlet<> m_out{this, "(signal) cleaned signal e = y - estimated feedback (times the guard's gain)", "signal"};
    outlet<> m_gain_out{this, "(signal) the guard's post-reverb (bus-stage) gain, linear; 1 with the guard off",
                        "signal"};
    outlet<thread_check::scheduler, thread_action::fifo> m_ipc_out{this, "(float) IPC double-talk indicator, 0..1"};
    outlet<thread_check::scheduler, thread_action::fifo> m_conv_out{
        this, "(list) uncertainty_db shadow_ratio_db [guard_state gain_db unprotected strikes]; calibrate_done"};

    /// First creation argument is the feedback-path filter length in samples
    /// (default 2048); partitions = ceil(filter_length / block size).
    explicit mutap_afc(const atoms& args = {}) {
        if (!args.empty()) {
            m_filter_length = std::clamp(static_cast<long>(args[0]), k_min_block, k_max_filter_length);
        }
        std::lock_guard<std::mutex> lock(m_control_mutex);
        m_constructed = true;
        publish();
    }

    ~mutap_afc() {
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
                          description{"Enable adaptation. Off freezes the learned feedback-path estimate; "
                                      "cancellation keeps running with the frozen filter."},
                          setter{MIN_FUNCTION{
                              const bool value = args[0];
                              m_adapt.store(value, std::memory_order_relaxed);
                              return {value};
                          }}};

    attribute<bool> gate{this, "gate", true,
                         description{"Robustness layer for double-talk: scale the step size by IPC^2 and skip "
                                     "updates on near-end transients (transient freeze ratio 4). Changing it "
                                     "rebuilds the canceller from scratch (the learned filter resets). With "
                                     "@warp on, the IPC step scaling stays on even when @gate is off — the "
                                     "warped model requires it."},
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
                                     "packed bass partials defeat the speech model. Warp keeps IPC step scaling on "
                                     "regardless of @gate (the warped whitener requires it for room-robust stability). "
                                     "Changing it rebuilds the canceller from scratch (the learned filter resets)."},
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
                                       "the burst floor instead (burst hardening at some added-stable-gain cost). "
                                       "The IPC outlet reports 0 with the Kalman engine (its gating is internal). "
                                       "Changing it rebuilds the canceller from scratch (the learned filter resets)."},
                           setter{MIN_FUNCTION{
                               const bool                  value = args[0];
                               std::lock_guard<std::mutex> lock(m_control_mutex);
                               m_kalman = value;
                               if (m_constructed) {
                                   publish();
                               }
                               return {value};
                           }}};

    attribute<int> shadow{
        this, "shadow", 2,
        description{"Shadow comparator: partitions of a small fast Kalman canceller that adapts beside the main one "
                    "on the same prewhitened signals (0 = off; default 2, the measured configuration, about 1 % "
                    "of the canceller's cost at block 64 / 48 kHz / 1024 taps). The value is kept as requested "
                    "(0-4096) and clamped to the partition count, ceil(filter length / @block), each time the "
                    "canceller is built, so @shadow 16 @block 64 keeps 16. The right outlet's second value, "
                    "shadow_ratio_db, is the main / shadow residual power ratio: it rises toward and past 0 dB "
                    "when the fresh short estimate beats the long one, as after the feedback path moves; it reads "
                    "0 dB with @shadow 0. Its smoothing keeps a 51 ms time constant at any @block and sample rate. "
                    "It applies to both engines but was measured with the Kalman engine only (@kalman 1). It does "
                    "not change the cleaned output. Changing it rebuilds the canceller from scratch (the learned "
                    "filter resets)."},
        setter{MIN_FUNCTION{
            const long                  requested = args[0];
            std::lock_guard<std::mutex> lock(m_control_mutex);
            m_shadow = std::clamp(requested, 0L, k_max_partitions);
            if (m_constructed) {
                publish();
            }
            return {static_cast<int>(m_shadow)};
        }}};

    // ------------------------------------------------------------ the guard

    attribute<bool> guard{
        this, "guard", false,
        description{"The anti-howl safety layer (MuTap howl_guard, one microphone): a gain policy over the "
                    "canceller's convergence verdict and a howl detector on its residual, applied to the cleaned "
                    "output. From every (re)build it holds the output @arming dB down (ARMING) until the verdict "
                    "(shadow_ratio_db < @d_db AND uncertainty_db < @a_db) has held @hold seconds with the detector "
                    "quiet, then opens; it ducks @duck dB on a howl trip or a lost verdict, releases on the "
                    "verdict or on a re-arm timer (@rearm s x 2^strikes), lowers its restore level 3 dB per strike "
                    "and latches after 3. After 10 s in ARMING without a verdict it reports unprotected, and opens "
                    "only to @cap if one is set. Needs @kalman 1 and @shadow > 0 (it reads both statistics): "
                    "otherwise the object posts an error and runs unguarded. Default off: the object then behaves "
                    "exactly as without it. With the guard on, the right outlet's list is uncertainty_db "
                    "shadow_ratio_db guard_state gain_db unprotected strikes (guard_state one of arming, open, "
                    "open_capped, ducked, releasing, latched), and the second (signal) outlet carries the guard's "
                    "bus-stage gain: 1 except while ducked or releasing, where it is the duck relative to the "
                    "restore level - multiply a reverb's output by it (*~) to cut a charged reverb too. Changing "
                    "it rebuilds the canceller from scratch (the learned filter resets) and starts the guard in "
                    "ARMING."},
        setter{MIN_FUNCTION{
            const bool                  value = args[0];
            std::lock_guard<std::mutex> lock(m_control_mutex);
            m_guard = value;
            if (m_constructed) {
                publish();
            }
            return {value};
        }}};

    attribute<numbers> cap{
        this, "cap", numbers{},
        description{"The deployment gain cap in dB re the patch's operating gain (at most 0; MuTap's protocol sets "
                    "the dry limit - 6 dB), or none (the default; send 'cap none'). It is the only way a guard "
                    "opens without a verdict: after the 10 s ARMING timeout it opens to the cap (OPEN_CAPPED, "
                    "unprotected); with no cap it stays ARMING, @arming dB down, for as long as the verdict does "
                    "not declare. Removing it re-arms a mic running on it. Applied live at the next vector."},
        setter{MIN_FUNCTION{
            std::lock_guard<std::mutex> lock(m_control_mutex);
            if (args.empty() || args[0].type() == message_type::symbol_argument) {
                m_policy.cap_db.reset();
            }
            else if (const double value = args[0]; std::isfinite(value)) {
                m_policy.cap_db = std::clamp(value, -120.0, 0.0);
            }
            push_policy(false);
            return m_policy.cap_db ? atoms{*m_policy.cap_db} : atoms{};
        }},
        // MIN_GETTER_FUNCTION spelled out: clang-format cannot see the lambda
        // through the macro.
        getter{[this]() -> atoms {
            std::lock_guard<std::mutex> lock(m_control_mutex);
            return m_policy.cap_db ? atoms{*m_policy.cap_db} : atoms{symbol{"none"}};
        }}};

    attribute<number> d_db{
        this, "d_db", -1.235,
        description{"The verdict's shadow-ratio threshold, dB (-120..120; MuTap's factory calibration -1.235): "
                    "ok needs shadow_ratio_db below it. Setting it (or @a_db) replaces a soundcheck calibration: "
                    "the guard runs on the attribute values again. Applied live at the next vector."},
        setter{MIN_FUNCTION{
            return {set_policy_field(&guard_policy::d_db, args[0], -120.0, 120.0, true)};
        }}};

    attribute<number> a_db{
        this, "a_db", -23.842,
        description{"The verdict's uncertainty threshold, dB (-300..120; MuTap's factory calibration -23.842): ok "
                    "needs uncertainty_db below it. Setting it replaces a soundcheck calibration, as @d_db does. "
                    "Applied live at the next vector."},
        setter{MIN_FUNCTION{
            return {set_policy_field(&guard_policy::a_db, args[0], -300.0, 120.0, true)};
        }}};

    attribute<number> duck{
        this, "duck", 20.0,
        description{"How far a duck sits under the restore level, dB (0..120, default 20). Applied live."},
        setter{MIN_FUNCTION{
            return {set_policy_field(&guard_policy::duck_db, args[0], 0.0, 120.0, false)};
        }}};

    attribute<number> hold{
        this, "hold", 1.5,
        description{"The release hold, seconds (default 1.5): the verdict must hold ok this long, with the detector "
                    "quiet, to declare (leave ARMING) or to release a duck. Applied live."},
        setter{MIN_FUNCTION{
            return {set_policy_field(&guard_policy::release_hold_s, args[0], 0.0, k_max_seconds, false)};
        }}};

    attribute<number> arming{
        this, "arming", 30.0,
        description{"ARMING's attenuation, dB (0..120, default 30; at least the cap's when one is set). Applied "
                    "live."},
        setter{MIN_FUNCTION{
            return {set_policy_field(&guard_policy::arming_duck_db, args[0], 0.0, 120.0, false)};
        }}};

    attribute<number> rearm{
        this, "rearm", 5.0,
        description{"A duck re-arms (releases) on a timer after this many seconds x 2^strikes with the detector "
                    "quiet, when the verdict cannot recover (default 5). Applied live."},
        setter{MIN_FUNCTION{
            return {set_policy_field(&guard_policy::rearm_timeout_s, args[0], 0.0, k_max_seconds, false)};
        }}};

    attribute<number> cal_s{
        this, "cal_s", 30.0,
        description{"The soundcheck window, seconds (default 30, the protocol's track-through): 'calibrate' samples "
                    "the statistics this long, then ends by itself. Read when a soundcheck begins."},
        setter{MIN_FUNCTION{
            return {set_policy_field(&guard_policy::calibrate_s, args[0], 0.0, k_max_seconds, false)};
        }}};

    attribute<number> cal_margin_d{
        this, "cal_margin_d", 4.0,
        description{"The soundcheck's shadow-ratio margin, dB (default 4, MuTap's measurement): the calibrated "
                    "threshold is the window's median shadow_ratio_db plus this. Read when a soundcheck ends."},
        setter{MIN_FUNCTION{
            return {set_policy_field(&guard_policy::cal_d_margin_db, args[0], -120.0, 120.0, false)};
        }}};

    attribute<number> cal_margin_a{
        this, "cal_margin_a", 3.0,
        description{"The soundcheck's uncertainty margin, dB (default 3, MuTap's measurement): the calibrated "
                    "threshold is the window's median uncertainty_db plus this. Read when a soundcheck ends."},
        setter{MIN_FUNCTION{
            return {set_policy_field(&guard_policy::cal_a_margin_db, args[0], -120.0, 120.0, false)};
        }}};

    /// Zero the learned filter and the block buffers, and return the guard to
    /// ARMING (applied on the audio thread at the next vector, so it does not
    /// race the perform routine).
    message<> reset{this, "reset",
                    "Reset the canceller: zero the learned feedback-path estimate (and return the guard to ARMING).",
                    MIN_FUNCTION{
                        m_reset_request.store(true, std::memory_order_relaxed);
                        return {};
                    }};

    /// Start the soundcheck (or, with 0, end it early). The audio thread
    /// begins the guard's sampler at the next vector; when it ends — after
    /// @cal_s, or on 'calibrate 0' — the guard runs on the calibrated
    /// thresholds and the right outlet sends 'calibrate_done d_db a_db'.
    message<> calibrate{
        this, "calibrate",
        "Soundcheck: sample the convergence statistics for @cal_s seconds, then run the guard on thresholds of "
        "median + margin (@cal_margin_d for the shadow ratio, @cal_margin_a for the uncertainty) and send "
        "'calibrate_done d_db a_db' out of the right outlet. 'calibrate 0' ends it early. Needs the guard running "
        "(@guard 1 with @kalman 1 and @shadow > 0); otherwise the object posts an error. The calibrated thresholds "
        "last until the next rebuild or until @d_db or @a_db is set.",
        MIN_FUNCTION{
            const bool begin = args.empty() || static_cast<bool>(args[0]);
            {
                std::lock_guard<std::mutex> lock(m_control_mutex);
                if (!m_engine_guard) {
                    cerr << "calibrate: the guard is not running (@guard 1 needs @kalman 1 and @shadow > 0)" << endl;
                    return {};
                }
            }
            m_calibration_request.store(begin ? calibration_request::begin : calibration_request::end,
                                        std::memory_order_relaxed);
            return {};
        }};

    /// Clear the guard's back-off (strikes, restore level, latch).
    message<> clear{this, "clear", "Clear the guard's back-off: strikes 0, restore level 0 dB, latch released.",
                    MIN_FUNCTION{
                        m_clear_request.store(true, std::memory_order_relaxed);
                        return {};
                    }};

    /// The shadow comparator's smoothing and the guard's timers are scaled for
    /// the sample rate, so a rate change while @shadow is on rebuilds the
    /// canceller (with the shadow off the engines are rate-agnostic and keep
    /// running; the guard needs the shadow). From the first call on, a
    /// requested guard the engine cannot host is reported. min-api sets
    /// samplerate() before calling this.
    message<> dspsetup{this, "dspsetup",
                       MIN_FUNCTION{
                           std::lock_guard<std::mutex> lock(m_control_mutex);
                           m_dsp_seen = true;
                           if (m_constructed && m_shadow != 0 && static_cast<double>(args[0]) != m_engine_sr) {
                               publish();
                           }
                           else if (m_guard && !guard_supported()) {
                               post_guard_refusal();
                           }
                           return {};
                       }};

    /// Shadow partitions the most recently built canceller was configured
    /// with (after the clamp to the partition count). Control-side state,
    /// read on the thread that sets attributes; the unit tests use it.
    size_t built_shadow_partitions() const {
        std::lock_guard<std::mutex> lock(m_control_mutex);
        return m_engine_shadow;
    }

    /// Whether the most recently built engine carries a guard. Control-side
    /// state, as built_shadow_partitions(); the unit tests use it.
    bool built_guard() const {
        std::lock_guard<std::mutex> lock(m_control_mutex);
        return m_engine_guard;
    }

    void operator()(audio_bundle input, audio_bundle output) {
        const auto    frames = input.frame_count();
        const double* y_in   = input.samples(0);
        const double* u_in   = input.samples(1);
        double*       out    = output.samples(0);
        double*       gain   = output.samples(1);

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
        // dry microphone signal through, at unity gain.
        if (!m_active) {
            for (auto i = 0; i < frames; ++i) {
                const double y = y_in[i];
                out[i]         = y;
                gain[i]        = 1.0;
            }
            return;
        }

        engine& eng = *m_active;
        guard_controls(eng);
        std::visit(
            [&](auto& afc) {
                afc.set_adaptation(m_adapt.load(std::memory_order_relaxed));
                if constexpr (requires { afc.fdaf().set_step_size(0.0); }) {
                    afc.fdaf().set_step_size(m_mu.load(std::memory_order_relaxed));
                }
                if (m_reset_request.exchange(false, std::memory_order_relaxed)) {
                    afc.reset();
                    std::fill(eng.u_block.begin(), eng.u_block.end(), 0.0);
                    std::fill(eng.y_block.begin(), eng.y_block.end(), 0.0);
                    std::fill(eng.e_block.begin(), eng.e_block.end(), 0.0);
                    std::fill(eng.g_block.begin(), eng.g_block.end(), 1.0);
                    eng.fill = 0;
                    // A cleared canceller behind an open guard is the
                    // full-gain cold start ARMING exists to prevent
                    // (afc_chain::reset() does the same).
                    if (eng.guard) {
                        eng.guard->reset();
                    }
                }

                // Vector-size bridging: gather into the block buffers, process
                // every time a block fills, play the processed block back out —
                // exactly block_size samples of latency, for host vectors smaller
                // or larger than the block. Inputs are read before the outputs are
                // written because Max may alias output buffers onto input buffers.
                const size_t b = afc.block_size();
                for (auto i = 0; i < frames; ++i) {
                    const double u        = u_in[i];
                    const double y        = y_in[i];
                    out[i]                = eng.e_block[eng.fill];
                    gain[i]               = eng.g_block[eng.fill];
                    eng.u_block[eng.fill] = u;
                    eng.y_block[eng.fill] = y;
                    if (++eng.fill == b) {
                        afc.process_block(eng.u_block.data(), eng.y_block.data(), eng.e_block.data());
                        eng.fill = 0;
                        // The guard needs both statistics: it is only ever
                        // built on the Kalman cores (guard_supported()).
                        if constexpr (requires { afc.uncertainty_ratio(); }) {
                            if (eng.guard) {
                                guard_block(eng, afc.uncertainty_ratio(), afc.shadow_residual_ratio());
                            }
                        }
                        if (--m_report_countdown <= 0) {
                            m_report_countdown = k_ipc_report_blocks;
                            // Right to left, Max's outlet order. The NLMS core
                            // keeps no state uncertainty: report 1 (0 dB),
                            // "nothing identified", the conservative reading.
                            // The shadow ratio is exactly 1 with @shadow 0.
                            double uncertainty = 1.0;
                            if constexpr (requires { afc.uncertainty_ratio(); }) {
                                uncertainty = afc.uncertainty_ratio();
                            }
                            report(eng, uncertainty, afc.shadow_residual_ratio());
                            m_ipc_atoms[0] = afc.ipc();
                            m_ipc_out.send(m_ipc_atoms);
                        }
                    }
                }
            },
            eng.afc);
    }

  private:
    /// Longest time any of the guard's timing attributes accepts, seconds
    /// (howl_guard clamps its policy to the same bound).
    static constexpr double k_max_seconds = 1e6;

    /// 10 log10 of a linear power ratio, floored at k_stat_floor so digital
    /// silence and a zero ratio stay finite. Audio thread; allocation-free.
    static double to_db(double ratio) noexcept { return 10.0 * std::log10(std::max(ratio, k_stat_floor)); }

    // ------------------------------------------------- the guard, audio side

    /// Top of every vector: the pending policy, then the host's requests
    /// (calibrate, clear). Requests that arrive while the engine has no guard
    /// are dropped, so a stale one never reaches a guard built later.
    void guard_controls(engine& eng) noexcept {
        if (m_policy_dirty.load(std::memory_order_acquire) && !m_policy_busy.test_and_set(std::memory_order_acquire)) {
            const policy_update update = m_policy_slot;
            m_policy_slot.thresholds   = false;
            m_policy_dirty.store(false, std::memory_order_relaxed);
            m_policy_busy.clear(std::memory_order_release);
            if (eng.guard) {
                eng.guard->set_policy(update.policy);
                if (update.thresholds) {
                    // @d_db / @a_db replace a soundcheck's thresholds.
                    eng.guard->clear_calibration();
                }
            }
        }
        const calibration_request request =
            m_calibration_request.exchange(calibration_request::none, std::memory_order_relaxed);
        const bool clear_request = m_clear_request.exchange(false, std::memory_order_relaxed);
        if (!eng.guard) {
            return;
        }
        if (request == calibration_request::begin) {
            eng.guard->calibrate_begin();
            eng.calibrating = true;
        }
        else if (request == calibration_request::end && eng.calibrating) {
            finish_calibration(eng);
        }
        if (clear_request) {
            eng.guard->clear();
        }
    }

    /// One guard tick on the block just processed: analyse the canceller's
    /// own residual and statistics, update, apply the mic's gain in place, and
    /// keep the bus-stage gain for the gain outlet. A soundcheck the guard
    /// ended by itself (after @cal_s) is applied and reported here.
    void guard_block(engine& eng, double uncertainty, double shadow) noexcept {
        guard_type& g = *eng.guard;
        g.analyze(0, eng.e_block.data(), uncertainty, shadow);
        g.update();
        g.apply(0, eng.e_block.data(), eng.e_block.data());
        const double* bus = g.bus_gain_block();
        std::copy(bus, bus + eng.g_block.size(), eng.g_block.begin());
        if (eng.calibrating && !g.calibrating()) {
            finish_calibration(eng);
        }
    }

    /// End the soundcheck: the guard runs on median + margin from the next
    /// block (calibrate_end(true); a window with no block keeps the current
    /// thresholds), and the right outlet says which: calibrate_done d_db a_db.
    void finish_calibration(engine& eng) noexcept {
        guard_type& g = *eng.guard;
        g.calibrate_end(true);
        eng.calibrating       = false;
        m_calibrated_atoms[1] = g.threshold_d_db(0);
        m_calibrated_atoms[2] = g.threshold_a_db(0);
        m_conv_out.send(m_calibrated_atoms);
    }

    /// The right outlet's report: the two statistics, and with a guard its
    /// state, gain (dB, end of the block), unprotected flag and strikes.
    void report(const engine& eng, double uncertainty, double shadow) noexcept {
        if (!eng.guard) {
            m_conv_atoms[0] = to_db(uncertainty);
            m_conv_atoms[1] = to_db(shadow);
            m_conv_out.send(m_conv_atoms);
            return;
        }
        const guard_type& g = *eng.guard;
        m_guard_atoms[0]    = to_db(uncertainty);
        m_guard_atoms[1]    = to_db(shadow);
        m_guard_atoms[2]    = m_state_names[static_cast<size_t>(g.state(0))];
        m_guard_atoms[3]    = g.gain_db(0);
        m_guard_atoms[4]    = g.unprotected(0) ? 1 : 0;
        m_guard_atoms[5]    = static_cast<long>(g.strikes(0));
        m_conv_out.send(m_guard_atoms);
    }

    // ----------------------------------------------- the guard, control side

    /// Whether the engine the current control state builds can host the
    /// guard: it reads uncertainty_ratio() (the Kalman core's) and the shadow
    /// comparator's ratio, as afc_chain::set_guard() requires.
    bool guard_supported() const { return m_kalman && shadow_in_use() > 0; }

    void post_guard_refusal() {
        cerr << "@guard 1 needs @kalman 1 and @shadow > 0 (the guard reads the Kalman core's uncertainty and the "
                "shadow comparator's ratio); running without the guard"
             << endl;
    }

    /// Hand the attributes' policy to the audio thread (applied at the top of
    /// the next vector). `thresholds`: @d_db or @a_db changed, so a soundcheck
    /// calibration is replaced. Caller holds m_control_mutex. The audio thread
    /// holds the flag only for one struct copy.
    void push_policy(bool thresholds) {
        while (m_policy_busy.test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        m_policy_slot.policy     = m_policy;
        m_policy_slot.thresholds = m_policy_slot.thresholds || thresholds;
        m_policy_dirty.store(true, std::memory_order_release);
        m_policy_busy.clear(std::memory_order_release);
    }

    /// Set one double field of the policy (clamped; a non-finite value keeps
    /// the current one, as howl_guard does), hand it over, and return what the
    /// guard will run on.
    double set_policy_field(double guard_policy::*field, double requested, double lo, double hi, bool thresholds) {
        std::lock_guard<std::mutex> lock(m_control_mutex);
        if (std::isfinite(requested)) {
            m_policy.*field = std::clamp(requested, lo, hi);
        }
        push_policy(thresholds);
        return m_policy.*field;
    }

    /// The guard's configuration for one engine: one microphone at the
    /// canceller's block and the DSP rate, the attributes' policy, and the
    /// two detector fields the geometry forces (MuTap's defaults otherwise):
    /// the band range under k_detector_top x fs, and the growth fit spanning
    /// at least the 3 blocks howl_detector requires (its 21.3 ms default holds
    /// up to @block 256 at 44.1 / 48 kHz).
    guard_type::config make_guard_config(size_t block, double sample_rate) const {
        guard_type::config cfg;
        cfg.microphones              = 1;
        cfg.block_size               = block;
        cfg.sample_rate              = sample_rate;
        cfg.policy                   = m_policy;
        cfg.detector.f_hi_hz         = std::min(cfg.detector.f_hi_hz, k_detector_top * sample_rate);
        const double three_blocks    = 3.0 * static_cast<double>(block) / sample_rate * (1.0 + 1e-9);
        cfg.detector.growth_window_s = std::max(cfg.detector.growth_window_s, three_blocks);
        return cfg;
    }

    /// Partition count of the main filter for the current control state.
    size_t partition_count() const {
        const auto b = static_cast<size_t>(m_block_size);
        return std::max<size_t>(1, (static_cast<size_t>(m_filter_length) + b - 1) / b);
    }

    /// Shadow partitions the next build uses: the request, clamped to the
    /// main filter's partition count (the setter bounds it only by
    /// k_max_partitions, so @block and the filter length can change the
    /// count in either direction without losing the request).
    size_t shadow_in_use() const { return std::min(static_cast<size_t>(m_shadow), partition_count()); }

    /// The shadow comparator's fields, shared by both core families: the
    /// clamped partition count and the per-block power retention
    /// beta = exp(-block / (fs * tau)) that holds tau at k_shadow_tau_s.
    template <typename Config>
    void set_shadow(Config& cfg, double sample_rate) const {
        cfg.shadow_partitions = shadow_in_use();
        cfg.shadow_smoothing  = std::exp(-static_cast<double>(cfg.fdaf.block_size) / (sample_rate * k_shadow_tau_s));
    }

    /// Assemble a pem_afc config from the current control-side state; the two
    /// instantiations share every field this external sets (the predictor
    /// configs differ, but both have analysis_capacity). The clamping in the
    /// setters keeps every constraint satisfied, so the canceller constructor
    /// does not throw for any reachable combination.
    template <typename Afc>
    typename Afc::config make_config(double sample_rate) const {
        typename Afc::config cfg;
        const auto           b          = static_cast<size_t>(m_block_size);
        cfg.fdaf.block_size             = b;
        cfg.fdaf.partitions             = partition_count();
        cfg.fdaf.step_size              = m_mu.load(std::memory_order_relaxed);
        cfg.fdaf.ipc_step_scaling       = m_gate || m_warp; // the warped whitener requires the IPC scale
        cfg.fdaf.transient_freeze_ratio = m_gate ? 4.0 : 0.0;
        // analysis_window must be a multiple of block_size and >= 2 * block_size;
        // both operands are powers of two, so the max is always a multiple.
        cfg.analysis_window             = std::max<size_t>(2 * b, 1024);
        cfg.predictor.analysis_capacity = std::max(cfg.predictor.analysis_capacity, cfg.analysis_window);
        set_shadow(cfg, sample_rate);
        return cfg;
    }

    /// Same, for the Kalman-core instantiations: no step size and no IPC
    /// options exist; @gate maps to the opt-in transient (burst) floor.
    template <typename Afc>
    typename Afc::config make_kalman_config(double sample_rate) const {
        typename Afc::config cfg;
        const auto           b          = static_cast<size_t>(m_block_size);
        cfg.fdaf.block_size             = b;
        cfg.fdaf.partitions             = partition_count();
        cfg.fdaf.transient_floor_ratio  = m_gate ? 8.0 : 0.0;
        cfg.analysis_window             = std::max<size_t>(2 * b, 1024);
        cfg.predictor.analysis_capacity = std::max(cfg.predictor.analysis_capacity, cfg.analysis_window);
        set_shadow(cfg, sample_rate);
        return cfg;
    }

    /// Build a canceller from the current config and publish it for the audio
    /// thread to adopt. Caller holds m_control_mutex.
    void publish() {
        delete m_trash.exchange(nullptr, std::memory_order_acq_rel); // reap
        try {
            // min-api initializes samplerate() to the global rate and updates
            // it to the chain's rate before dspsetup.
            const double            sr = samplerate() > 0.0 ? samplerate() : 48000.0;
            std::unique_ptr<engine> eng;
            if (m_kalman) {
                eng = m_warp ? std::make_unique<engine>(std::in_place_type<kalman_warped_afc>,
                                                        make_kalman_config<kalman_warped_afc>(sr))
                             : std::make_unique<engine>(std::in_place_type<kalman_speech_afc>,
                                                        make_kalman_config<kalman_speech_afc>(sr));
            }
            else {
                eng = m_warp ? std::make_unique<engine>(std::in_place_type<warped_afc>, make_config<warped_afc>(sr))
                             : std::make_unique<engine>(std::in_place_type<speech_afc>, make_config<speech_afc>(sr));
            }
            // The guard, at the canceller's block and this rate. A request the
            // engine cannot host is reported once DSP has been set up (box
            // attributes arrive one at a time before that: '@guard 1 @kalman 1'
            // passes through a guard-without-Kalman state on its way).
            const bool guarded = m_guard && guard_supported();
            if (guarded) {
                eng->guard.emplace(make_guard_config(static_cast<size_t>(m_block_size), sr));
            }
            else if (m_guard && m_dsp_seen) {
                post_guard_refusal();
            }
            m_engine_sr     = sr;
            m_engine_shadow = shadow_in_use();
            m_engine_guard  = guarded;
            // A still-unadopted previous pending engine comes back to us here
            // and is deleted — the audio thread only ever sees the newest one.
            delete m_pending.exchange(eng.release(), std::memory_order_acq_rel);
        }
        catch (const std::exception& ex) {
            // Defensive: leave the current canceller running (the perform path
            // falls back to the dry microphone signal if none exists yet) and
            // say why. The setters' clamping keeps every configuration MuTap's
            // validation (the FFT size gate included) accepts, so this is not
            // expected to fire.
            cerr << "engine rebuild failed: " << ex.what() << endl;
        }
    }
};

MIN_EXTERNAL(mutap_afc);
