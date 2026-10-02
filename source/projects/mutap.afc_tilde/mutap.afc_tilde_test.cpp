/// @file
/// @brief      Unit tests for mutap.afc~ (Min-level: attribute defaults, rounding, clamping,
///             the convergence outlet's structure, and the guard's glue: requirements,
///             policy hand-over, the signal and gain outlets, calibrate / cap / reset / clear).
// SPDX-License-Identifier: MIT
// Copyright 2026 MuTap contributors

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <string>
#include <thread>
#include <vector>

#include "c74_min_unittest.h"  // required unit-test header (defines main via Catch)
#include "mutap.afc_tilde.cpp" // include the object source so we can instantiate it

namespace {

    constexpr int  k_ipc_outlet  = 2;
    constexpr int  k_conv_outlet = 3;
    constexpr long k_vector_size = 64; ///< host vector size the tests drive (smaller than every @block used)

    /// Open-loop stand-in for the room: the reference u is deterministic white
    /// noise and the microphone hears it through a 7-sample delay at half
    /// amplitude, so every engine has a path to identify; `tone` adds a 1 kHz
    /// near-end sine of that amplitude to the microphone (0 = none). The
    /// state carries across perform calls, and both signal outlets are
    /// recorded. The loop is open: what the object outputs never reaches u,
    /// so the canceller's residual does not depend on the guard.
    struct room {
        static constexpr size_t k_delay = 7;

        std::uint32_t                     seed{0x2545F491U};
        std::array<double, k_delay>       history{};
        size_t                            head{0};
        double                            tone{0.0};
        double                            phase{0.0};
        std::array<double, k_vector_size> u{};
        std::array<double, k_vector_size> y{};
        std::array<double, k_vector_size> e{};
        std::array<double, k_vector_size> g{};
        std::vector<double>               out;  ///< signal outlet 0, every sample driven
        std::vector<double>               gain; ///< signal outlet 1, every sample driven

        /// Run `samples` samples (a multiple of k_vector_size) through the object's perform routine.
        void drive(mutap_afc& obj, long samples) {
            const double step = 2.0 * std::numbers::pi * 1000.0 / obj.samplerate();
            for (long done = 0; done < samples; done += k_vector_size) {
                for (size_t i = 0; i < u.size(); ++i) {
                    seed          = seed * 1664525U + 1013904223U;
                    u[i]          = static_cast<double>(seed >> 8) / static_cast<double>(1U << 24) - 0.5;
                    phase         = std::fmod(phase + step, 2.0 * std::numbers::pi);
                    y[i]          = 0.5 * history[head] + tone * std::sin(phase);
                    history[head] = u[i];
                    head          = (head + 1) % k_delay;
                }
                std::array<double*, 2> in{y.data(), u.data()};
                std::array<double*, 2> outs{e.data(), g.data()};
                obj(audio_bundle{in.data(), 2, k_vector_size}, audio_bundle{outs.data(), 2, k_vector_size});
                out.insert(out.end(), e.begin(), e.end());
                gain.insert(gain.end(), g.begin(), g.end());
            }
        }
    };

    /// The mock kernel's global sample rate (sys_getsr()), which every
    /// instance starts with; the guard scenarios check it.
    constexpr double k_mock_rate = 44100.0;

    /// Whole vectors in `seconds` at the mock's rate.
    long seconds_of(double seconds) {
        return static_cast<long>(seconds * k_mock_rate) / k_vector_size * k_vector_size;
    }

    /// The IPC and convergence outlets are fifo outlets bound to the scheduler
    /// thread; from the test's (main) thread they queue, and the mock kernel
    /// delivers each on its own clock thread. Wait until the convergence
    /// outlet has logged `conv_count` messages and the IPC outlet
    /// `ipc_count`, then give the clock threads' remaining (empty) deliveries
    /// time to run before the object can be freed. The mock offers no
    /// synchronization with those threads, so this polls. False when the
    /// deadline passed first.
    bool wait_for_reports(mutap_afc& obj, size_t conv_count, size_t ipc_count) {
        const c74::max::t_sequence& conv     = *c74::max::object_getoutput(obj.maxobj(), k_conv_outlet);
        const c74::max::t_sequence& ipc      = *c74::max::object_getoutput(obj.maxobj(), k_ipc_outlet);
        const auto                  deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while ((conv.size() < conv_count || ipc.size() < ipc_count) && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return conv.size() >= conv_count && ipc.size() >= ipc_count;
    }

    /// What one run left behind: the convergence outlet's log and the two
    /// signal outlets.
    struct run_log {
        c74::max::t_sequence reports;
        std::vector<double>  out;
        std::vector<double>  gain;
    };

    /// Build a default instance, let `run` configure it and drive it through
    /// a room, and return its log once the convergence outlet has sent
    /// `conv_count` messages and the IPC outlet `ipc_count` (empty if they
    /// never did). A fresh instance is tried up to three times: min-api's mock
    /// clock (test/mock/c74_mock_clock.h) starts its thread in a member
    /// initializer that precedes the flag the thread loops on, and a thread
    /// that wins that race exits at once and never delivers. Measured on the
    /// Intel Mac with 16 test binaries running at once: before this retry, 8
    /// of 80 binary runs failed on a missing delivery; with it, 36 first
    /// attempts failed over 160 binary runs, no second attempt did, and all
    /// 160 passed. Run one at a time, 0 of 30 binary runs needed a retry. Max
    /// has no such thread; an external that never sends fails every attempt.
    template <typename Run>
    run_log run_logged(size_t conv_count, size_t ipc_count, Run&& run) {
        for (int attempt = 0; attempt < 3; ++attempt) {
            test_wrapper<mutap_afc> an_instance;
            mutap_afc&              my_object = an_instance;
            room                    the_room;
            run(my_object, the_room);
            if (wait_for_reports(my_object, conv_count, ipc_count)) {
                return {*c74::max::object_getoutput(my_object.maxobj(), k_conv_outlet), the_room.out, the_room.gain};
            }
        }
        return {};
    }

    /// The convergence outlet's log once `count` reports arrived on both
    /// report outlets (run_logged() without the signals).
    template <typename Run>
    c74::max::t_sequence reports_from(size_t count, Run&& run) {
        return run_logged(count, count, std::forward<Run>(run)).reports;
    }

    double value(const atom& a) {
        return static_cast<double>(a);
    }

    /// The symbol in a report element ("" when it is not one).
    std::string name_of(const atom& a) {
        return a.a_type == c74::max::A_SYM ? std::string(atom_getsym(&a)->s_name) : std::string{};
    }

    /// The guard's six-element reports, in order (calibrate_done left out).
    std::vector<c74::max::t_atom_vector> guard_reports(const c74::max::t_sequence& log) {
        std::vector<c74::max::t_atom_vector> reports;
        for (const auto& r : log) {
            if (r.size() == 6) {
                reports.push_back(r);
            }
        }
        return reports;
    }

    /// The calibrate_done messages, in order.
    std::vector<c74::max::t_atom_vector> calibrations(const c74::max::t_sequence& log) {
        std::vector<c74::max::t_atom_vector> done;
        for (const auto& r : log) {
            if (!r.empty() && name_of(r[0]) == "calibrate_done") {
                done.push_back(r);
            }
        }
        return done;
    }

} // namespace

SCENARIO("mutap.afc~ instantiates with the documented defaults") {
    ext_main(nullptr); // configure the class (required once per test executable)

    GIVEN("a default instance") {
        test_wrapper<mutap_afc> an_instance;
        mutap_afc&              my_object = an_instance;

        THEN("the canceller geometry defaults to a 256-sample block") {
            REQUIRE(static_cast<int>(my_object.block) == 256);
        }
        THEN("the NLMS step size defaults to the robust 0.5") {
            REQUIRE(static_cast<double>(my_object.mu) == 0.5);
        }
        THEN("adaptation and the double-talk gate are on") {
            REQUIRE(static_cast<bool>(my_object.adapt) == true);
            REQUIRE(static_cast<bool>(my_object.gate) == true);
        }
        THEN("the speech near-end model and the NLMS core are selected") {
            REQUIRE(static_cast<bool>(my_object.warp) == false);
            REQUIRE(static_cast<bool>(my_object.kalman) == false);
        }
        THEN("the shadow comparator is on at the measured 2 partitions") {
            REQUIRE(static_cast<int>(my_object.shadow) == 2);
        }
        THEN("the guard is off, and none was built") {
            REQUIRE(static_cast<bool>(my_object.guard) == false);
            REQUIRE(my_object.built_guard() == false);
        }
        THEN("the guard's policy attributes carry MuTap's guard_policy defaults, and no cap") {
            const tap::mu::guard_policy factory;
            REQUIRE(static_cast<double>(my_object.d_db) == factory.d_db);
            REQUIRE(static_cast<double>(my_object.a_db) == factory.a_db);
            REQUIRE(static_cast<double>(my_object.duck) == factory.duck_db);
            REQUIRE(static_cast<double>(my_object.hold) == factory.release_hold_s);
            REQUIRE(static_cast<double>(my_object.arming) == factory.arming_duck_db);
            REQUIRE(static_cast<double>(my_object.rearm) == factory.rearm_timeout_s);
            REQUIRE(static_cast<double>(my_object.cal_s) == factory.calibrate_s);
            REQUIRE(static_cast<double>(my_object.cal_margin_d) == factory.cal_d_margin_db);
            REQUIRE(static_cast<double>(my_object.cal_margin_a) == factory.cal_a_margin_db);
            REQUIRE_FALSE(factory.cap_db.has_value());
            const atoms cap = my_object.cap;
            REQUIRE(cap.size() == 1);
            REQUIRE(name_of(cap[0]) == "none");
        }
    }
}

SCENARIO("mutap.afc~ rounds the block size up to a power of two") {
    ext_main(nullptr);

    GIVEN("a default instance") {
        test_wrapper<mutap_afc> an_instance;
        mutap_afc&              my_object = an_instance;

        WHEN("a non-power-of-two block is requested") {
            my_object.block = 100;
            THEN("it rounds up rather than down") {
                REQUIRE(static_cast<int>(my_object.block) == 128);
            }
        }
        WHEN("an exact power of two is requested") {
            my_object.block = 512;
            THEN("it is taken as-is") {
                REQUIRE(static_cast<int>(my_object.block) == 512);
            }
        }
        WHEN("a block below the 16-sample floor is requested") {
            my_object.block = 1;
            THEN("it clamps up to the floor") {
                REQUIRE(static_cast<int>(my_object.block) == 16);
            }
        }
        WHEN("a block above the 4096-sample ceiling is requested") {
            my_object.block = 99999;
            THEN("it clamps down to the ceiling") {
                REQUIRE(static_cast<int>(my_object.block) == 4096);
            }
        }
    }
}

SCENARIO("mutap.afc~ clamps the adaptation step size into the stable open interval") {
    ext_main(nullptr);

    GIVEN("a default instance") {
        test_wrapper<mutap_afc> an_instance;
        mutap_afc&              my_object = an_instance;

        WHEN("mu is pushed past the documented (0, 2) range") {
            my_object.mu = 5.0;
            THEN("it clamps just below 2") {
                REQUIRE(static_cast<double>(my_object.mu) == 1.99);
            }
        }
        WHEN("mu is set to zero") {
            my_object.mu = 0.0;
            THEN("it clamps to the small positive floor, never zero") {
                REQUIRE(static_cast<double>(my_object.mu) == 0.001);
            }
        }
        WHEN("mu is set inside the range") {
            my_object.mu = 0.25;
            THEN("it is taken as-is") {
                REQUIRE(static_cast<double>(my_object.mu) == 0.25);
            }
        }
    }
}

// Every k_ipc_report_blocks (8) processed blocks the right outlet sends
// "uncertainty_db shadow_ratio_db". These are structural checks of the glue;
// the statistics themselves are gated in MuTap (tests/test_convergence_stats.cpp).

SCENARIO("mutap.afc~ reports the convergence statistics as a two-element list") {
    ext_main(nullptr);

    GIVEN("a default instance (NLMS core, @shadow 2) driven for 64 blocks of 256") {
        const auto reports =
            reports_from(8, [](mutap_afc& my_object, room& the_room) { the_room.drive(my_object, 64 * 256); });

        THEN("8 reports arrive, each two finite numbers") {
            REQUIRE(reports.size() == 8);
            for (const auto& r : reports) {
                REQUIRE(r.size() == 2);
                REQUIRE(std::isfinite(value(r[0])));
                REQUIRE(std::isfinite(value(r[1])));
            }
        }
        THEN("the NLMS core, which keeps no uncertainty, reads 0 dB (nothing identified)") {
            REQUIRE(reports.size() == 8);
            for (const auto& r : reports) {
                REQUIRE(value(r[0]) == 0.0);
            }
        }
    }
}

SCENARIO("mutap.afc~'s uncertainty falls as the Kalman core identifies the path") {
    ext_main(nullptr);

    GIVEN("an instance on the Kalman core driven for 200 blocks of 256") {
        const auto reports = reports_from(25, [](mutap_afc& my_object, room& the_room) {
            my_object.kalman = true;
            the_room.drive(my_object, 200 * 256);
        });

        THEN("the last report's uncertainty is below 0 dB") {
            REQUIRE(reports.size() == 25);
            INFO("uncertainty_db " << value(reports.back()[0]) << ", shadow_ratio_db " << value(reports.back()[1]));
            // Measured: -40.0546 dB (shadow_ratio_db -0.598193); deterministic, a fixed
            // noise seed in an open loop. The assertion is only the direction.
            REQUIRE(value(reports.back()[0]) < 0.0);
        }
    }
}

SCENARIO("mutap.afc~ reads 0 dB for the shadow ratio with the shadow off") {
    ext_main(nullptr);

    GIVEN("an instance on the Kalman core with @shadow 0, driven for 64 blocks of 256") {
        int        shadow_read = -1;
        const auto reports     = reports_from(8, [&shadow_read](mutap_afc& my_object, room& the_room) {
            my_object.shadow = 0;
            my_object.kalman = true;
            shadow_read      = static_cast<int>(my_object.shadow);
            the_room.drive(my_object, 64 * 256);
        });

        THEN("every report's shadow ratio is exactly 0 dB") {
            REQUIRE(shadow_read == 0);
            REQUIRE(reports.size() == 8);
            for (const auto& r : reports) {
                REQUIRE(value(r[1]) == 0.0);
            }
        }
    }
}

SCENARIO("mutap.afc~ keeps the requested @shadow and clamps it to the partition count at build time") {
    ext_main(nullptr);

    GIVEN("a default instance (2048 taps at @block 256: 8 partitions)") {
        test_wrapper<mutap_afc> an_instance;
        mutap_afc&              my_object = an_instance;

        THEN("the default engine is built with the default 2 shadow partitions") {
            REQUIRE(my_object.built_shadow_partitions() == 2);
        }
        WHEN("@shadow is pushed past the partition count") {
            my_object.shadow = 100;
            THEN("the attribute keeps the request and the engine gets the partition count") {
                REQUIRE(static_cast<int>(my_object.shadow) == 100);
                REQUIRE(my_object.built_shadow_partitions() == 8);
            }
        }
        WHEN("@shadow 16 is set, then @block 64 (2048 taps: 32 partitions)") {
            my_object.shadow = 16;
            REQUIRE(my_object.built_shadow_partitions() == 8);
            my_object.block = 64;
            THEN("the request survives and the rebuilt engine has 16 shadow partitions") {
                REQUIRE(static_cast<int>(my_object.shadow) == 16);
                REQUIRE(my_object.built_shadow_partitions() == 16);
            }
        }
        WHEN("@shadow is pushed past the most partitions any geometry has (65536 taps / 16)") {
            my_object.shadow = 5000;
            THEN("it clamps to 4096") {
                REQUIRE(static_cast<int>(my_object.shadow) == 4096);
            }
        }
        WHEN("@shadow is set negative") {
            my_object.shadow = -3;
            THEN("it clamps to 0 (off)") {
                REQUIRE(static_cast<int>(my_object.shadow) == 0);
                REQUIRE(my_object.built_shadow_partitions() == 0);
            }
        }
    }
}

SCENARIO("mutap.afc~ rebuilds on @shadow and sample-rate changes and keeps reporting") {
    ext_main(nullptr);

    GIVEN("a default instance whose @shadow, @kalman, sample rate and @block change between runs") {
        // 5 runs of 16 blocks of 256, then 64 blocks of 64: 144 blocks, 18 reports.
        const auto reports = reports_from(18, [](mutap_afc& my_object, room& the_room) {
            the_room.drive(my_object, 16 * 256);
            my_object.shadow = 0;
            the_room.drive(my_object, 16 * 256);
            my_object.shadow = 8;
            the_room.drive(my_object, 16 * 256);
            my_object.kalman = true;
            my_object.shadow = 1;
            the_room.drive(my_object, 16 * 256);
            my_object.samplerate(96000.0);
            my_object.dspsetup(atoms{96000.0, k_vector_size});
            the_room.drive(my_object, 16 * 256);
            my_object.block = 64; // 32 partitions; @shadow 1 still fits
            the_room.drive(my_object, 64 * 64);
        });

        THEN("reports keep arriving as finite two-element lists") {
            REQUIRE(reports.size() == 18);
            for (const auto& r : reports) {
                REQUIRE(r.size() == 2);
                REQUIRE(std::isfinite(value(r[0])));
                REQUIRE(std::isfinite(value(r[1])));
            }
        }
    }
}

// ------------------------------------------------------------------ the guard
//
// The guard's DSP is MuTap's (tests/test_howl_guard.cpp pins every transition,
// tests/test_howl_guard_host.cpp the closed-loop claims). These scenarios
// check the glue: which engines carry it, that @guard 0 leaves the object as
// it was, the policy hand-over, the two signal outlets, and the messages. The
// room is open loop, so the canceller's residual is the same with and without
// the guard. In it the verdict never declares on the factory thresholds: the
// shadow ratio reads -0.26 to -0.55 dB at every 25th report of a 12 s run,
// above d_db -1.235 (measured, mock rate 44.1 kHz, @block 256, 2048 taps).

namespace {

    constexpr double k_block_256 = 256.0;
    constexpr size_t k_report    = 8; ///< blocks per report (k_ipc_report_blocks)

    /// Reports a run of `samples` at @block 256 sends.
    size_t reports_in(long samples) {
        return static_cast<size_t>(samples / 256) / k_report;
    }

    /// Start time of report k (0-based), seconds, at @block 256.
    double report_time(size_t k, double sample_rate) {
        return static_cast<double>((k + 1) * k_report) * k_block_256 / sample_rate;
    }

    double db_to_gain(double db) {
        return std::pow(10.0, db / 20.0);
    }

} // namespace

SCENARIO("mutap.afc~ clamps the guard's policy attributes to howl_guard's ranges") {
    ext_main(nullptr);

    GIVEN("a default instance") {
        test_wrapper<mutap_afc> an_instance;
        mutap_afc&              my_object = an_instance;

        WHEN("@cap is set inside the range, above 0 dB, below -120 dB, and back to none") {
            my_object.cap      = atoms{-12.0};
            const atoms inside = my_object.cap;
            my_object.cap      = atoms{5.0};
            const atoms above  = my_object.cap;
            my_object.cap      = atoms{-500.0};
            const atoms below  = my_object.cap;
            my_object.cap      = atoms{symbol{"none"}};
            const atoms none   = my_object.cap;
            my_object.cap      = atoms{-6.0};
            my_object.cap      = atoms{};
            const atoms empty  = my_object.cap;
            THEN("it reads back the cap the guard runs on") {
                REQUIRE(value(inside[0]) == -12.0);
                REQUIRE(value(above[0]) == 0.0);
                REQUIRE(value(below[0]) == -120.0);
                REQUIRE(name_of(none[0]) == "none");
                REQUIRE(name_of(empty[0]) == "none");
            }
        }
        WHEN("the dB and time attributes are pushed past their ranges") {
            my_object.d_db         = 500.0;
            my_object.a_db         = -1000.0;
            my_object.duck         = -3.0;
            my_object.arming       = 500.0;
            my_object.hold         = -1.0;
            my_object.rearm        = -2.0;
            my_object.cal_s        = -5.0;
            my_object.cal_margin_d = 300.0;
            my_object.cal_margin_a = -300.0;
            THEN("each clamps where howl_guard's set_policy() clamps") {
                REQUIRE(static_cast<double>(my_object.d_db) == 120.0);
                REQUIRE(static_cast<double>(my_object.a_db) == -300.0);
                REQUIRE(static_cast<double>(my_object.duck) == 0.0);
                REQUIRE(static_cast<double>(my_object.arming) == 120.0);
                REQUIRE(static_cast<double>(my_object.hold) == 0.0);
                REQUIRE(static_cast<double>(my_object.rearm) == 0.0);
                REQUIRE(static_cast<double>(my_object.cal_s) == 0.0);
                REQUIRE(static_cast<double>(my_object.cal_margin_d) == 120.0);
                REQUIRE(static_cast<double>(my_object.cal_margin_a) == -120.0);
            }
        }
    }
}

SCENARIO("mutap.afc~ builds the guard only on an engine with both statistics") {
    ext_main(nullptr);

    GIVEN("a default instance (the NLMS core, @shadow 2)") {
        test_wrapper<mutap_afc> an_instance;
        mutap_afc&              my_object = an_instance;

        WHEN("@guard 1 is set on the NLMS core") {
            my_object.guard = true;
            THEN("the request is kept but no guard is built (the NLMS core has no uncertainty)") {
                REQUIRE(static_cast<bool>(my_object.guard) == true);
                REQUIRE(my_object.built_guard() == false);
            }
            AND_WHEN("@kalman 1 follows") {
                my_object.kalman = true;
                THEN("the guard is built") {
                    REQUIRE(my_object.built_guard() == true);
                }
                AND_WHEN("@shadow 0 follows") {
                    my_object.shadow = 0;
                    THEN("the guard is dropped (the shadow ratio would read 0 dB for good)") {
                        REQUIRE(my_object.built_guard() == false);
                    }
                }
                AND_WHEN("@guard 0 follows") {
                    my_object.guard = false;
                    THEN("the guard is gone") {
                        REQUIRE(my_object.built_guard() == false);
                    }
                }
            }
        }
    }
    GIVEN("geometries outside the guard's measured one") {
        test_wrapper<mutap_afc> an_instance;
        mutap_afc&              my_object = an_instance;

        // howl_detector rejects both without the external's two adjustments
        // (a growth fit shorter than 3 blocks; a top band at or above Nyquist).
        WHEN("@block 4096 at 44.1 kHz") {
            my_object.block  = 4096;
            my_object.kalman = true;
            my_object.guard  = true;
            THEN("the guard is built (the detector's growth fit widened to 3 blocks)") {
                REQUIRE(my_object.built_guard() == true);
            }
        }
        WHEN("a 22.05 kHz signal chain") {
            my_object.samplerate(22050.0);
            my_object.kalman = true;
            my_object.guard  = true;
            THEN("the guard is built (the detector's top band lowered under Nyquist)") {
                REQUIRE(my_object.built_guard() == true);
            }
        }
    }
}

SCENARIO("mutap.afc~ with @guard 0 is the canceller alone") {
    ext_main(nullptr);

    GIVEN("a default instance driven for 64 blocks of 256, and pem_afc<double> configured as the object documents") {
        const auto off =
            run_logged(8, 8, [](mutap_afc& my_object, room& the_room) { the_room.drive(my_object, 64 * 256); });
        const auto refused = run_logged(8, 8, [](mutap_afc& my_object, room& the_room) {
            my_object.guard = true; // on the NLMS core: refused, runs unguarded
            the_room.drive(my_object, 64 * 256);
        });

        // The default engine: the speech cascade on the NLMS core, block 256,
        // 2048 taps, mu 0.5, the gate (IPC step scaling, transient freeze 4),
        // the 1024-sample analysis window and the 2-partition shadow at the
        // mock's 44.1 kHz.
        tap::mu::pem_afc<double>::config cfg;
        cfg.fdaf.block_size             = 256;
        cfg.fdaf.partitions             = 8;
        cfg.fdaf.step_size              = 0.5;
        cfg.fdaf.ipc_step_scaling       = true;
        cfg.fdaf.transient_freeze_ratio = 4.0;
        cfg.analysis_window             = 1024;
        cfg.predictor.analysis_capacity = std::max<size_t>(cfg.predictor.analysis_capacity, 1024);
        cfg.shadow_partitions           = 2;
        cfg.shadow_smoothing            = std::exp(-256.0 / (44100.0 * 0.051));
        tap::mu::pem_afc<double> reference(cfg);
        room                     source;             // the same seed: the same u and y
        std::vector<double>      expected(256, 0.0); // the object's one block of latency
        std::vector<double>      e(256);
        for (int block = 0; block < 63; ++block) {
            std::array<double, 256> u{};
            std::array<double, 256> y{};
            for (size_t i = 0; i < 256; ++i) {
                source.seed = source.seed * 1664525U + 1013904223U;
                u[i]        = static_cast<double>(source.seed >> 8) / static_cast<double>(1U << 24) - 0.5;
                y[i]        = 0.5 * source.history[source.head];
                source.history[source.head] = u[i];
                source.head                 = (source.head + 1) % room::k_delay;
            }
            reference.process_block(u.data(), y.data(), e.data());
            expected.insert(expected.end(), e.begin(), e.end());
        }

        THEN("the cleaned output is the reference's residual one block later, bit for bit") {
            REQUIRE(off.out.size() == expected.size());
            REQUIRE(off.out == expected);
        }
        THEN("a refused @guard 1 changes nothing either") {
            REQUIRE(refused.out == off.out);
        }
        THEN("the gain outlet is exactly 1 throughout") {
            REQUIRE(off.gain.size() == off.out.size());
            REQUIRE(std::all_of(off.gain.begin(), off.gain.end(), [](double g) { return g == 1.0; }));
            REQUIRE(std::all_of(refused.gain.begin(), refused.gain.end(), [](double g) { return g == 1.0; }));
        }
        THEN("the right outlet keeps its two-element list") {
            REQUIRE(off.reports.size() == 8);
            REQUIRE(refused.reports.size() == 8);
            for (const auto& r : off.reports) {
                REQUIRE(r.size() == 2);
            }
            for (const auto& r : refused.reports) {
                REQUIRE(r.size() == 2);
            }
        }
    }
}

SCENARIO("mutap.afc~'s guard holds ARMING's gain on the cleaned output while the verdict has not declared") {
    ext_main(nullptr);

    GIVEN("@kalman 1 with and without @guard 1, driven for 10.5 s through the same open-loop room") {
        // Catch re-enters the GIVEN once per THEN: run the pair once.
        struct runs {
            double  rate{0.0};
            size_t  count{0};
            run_log guarded;
            run_log plain;
        };
        static const runs k_runs = [] {
            runs       r;
            const long samples = seconds_of(10.5);
            r.count            = reports_in(samples);
            r.guarded          = run_logged(r.count, r.count, [&](mutap_afc& my_object, room& the_room) {
                r.rate           = my_object.samplerate();
                my_object.kalman = true;
                my_object.guard  = true;
                the_room.drive(my_object, samples);
            });
            r.plain            = run_logged(0, 0, [&](mutap_afc& my_object, room& the_room) {
                my_object.kalman = true;
                the_room.drive(my_object, samples);
            });
            return r;
        }();
        const run_log& guarded = k_runs.guarded;
        const run_log& plain   = k_runs.plain;
        const double   rate    = k_runs.rate;
        const size_t   count   = k_runs.count;
        const auto     reports = guard_reports(guarded.reports);

        THEN("the instance ran at the mock's rate, which the timings below assume") {
            REQUIRE(rate == k_mock_rate);
        }
        THEN("the cleaned output is the unguarded one at exactly ARMING's -30 dB") {
            REQUIRE(guarded.out.size() == plain.out.size());
            const double arming = db_to_gain(-30.0);
            double       worst  = 0.0; // largest |guarded - arming x plain| relative to |plain|
            for (size_t i = 0; i < plain.out.size(); ++i) {
                if (plain.out[i] != 0.0) {
                    worst = std::max(worst, std::abs(guarded.out[i] - arming * plain.out[i]) / std::abs(plain.out[i]));
                }
            }
            // Measured 0 over 463040 samples (the guard multiplies by the same
            // 10^(-30/20)); the bound only allows for a libm that differs.
            REQUIRE(worst < 1e-12);
        }
        THEN("the gain outlet stays at 1: the bus stage cuts only a duck") {
            REQUIRE(std::all_of(guarded.gain.begin(), guarded.gain.end(), [](double g) { return g == 1.0; }));
        }
        THEN("every report says arming at -30 dB with no strikes, and unprotected from the 10 s timeout on") {
            REQUIRE(reports.size() == count);
            // The timeout is ceil(10 s / (256 / 44100 s)) = 1723 blocks; report k
            // closes block 8 (k + 1), so report 214 (block 1720) is the last
            // before it and report 215 (block 1728) the first after.
            const size_t first_unprotected = 215;
            for (size_t k = 0; k < reports.size(); ++k) {
                const auto& r = reports[k];
                INFO("report " << k << " at " << report_time(k, rate) << " s");
                REQUIRE(name_of(r[2]) == "arming");
                REQUIRE(value(r[3]) == Approx(-30.0).margin(1e-9));
                REQUIRE(value(r[4]) == (k >= first_unprotected ? 1.0 : 0.0));
                REQUIRE(value(r[5]) == 0.0);
            }
        }
    }
}

SCENARIO("mutap.afc~'s @cap opens an undeclared guard at the timeout, and reset returns it to ARMING") {
    ext_main(nullptr);

    GIVEN("@kalman 1 @guard 1 @cap -12, driven for 10.5 s, then reset and 0.5 s more") {
        const long           before  = seconds_of(10.5);
        const long           after   = seconds_of(0.5);
        const size_t         count   = reports_in(before + after);
        static const run_log k_log   = run_logged(count, count, [before, after](mutap_afc& my_object, room& the_room) {
            my_object.kalman = true;
            my_object.guard  = true;
            my_object.cap    = atoms{-12.0};
            the_room.drive(my_object, before);
            my_object.reset();
            the_room.drive(my_object, after);
        });
        const auto           reports = guard_reports(k_log.reports);
        const size_t         last    = reports_in(before) - 1; // the last report before the reset

        THEN("it arms at -30 dB, opens capped at the timeout and settles on the cap, unprotected") {
            REQUIRE(reports.size() == count);
            for (size_t k = 0; k < 215; ++k) {
                REQUIRE(name_of(reports[k][2]) == "arming");
            }
            // Report 215 closes block 1728, the first after the 1723-block timeout.
            REQUIRE(name_of(reports[215][2]) == "open_capped");
            REQUIRE(value(reports[215][4]) == 1.0);
            // Measured: -26.865306 dB at report 215 (the 200 ms ramp up to the
            // cap), -12 at the last; the ramp lands exactly on its target.
            REQUIRE(name_of(reports[last][2]) == "open_capped");
            REQUIRE(value(reports[last][3]) == Approx(-12.0).margin(1e-9));
            REQUIRE(value(reports[last][4]) == 1.0);
        }
        THEN("the first report after reset is ARMING at -30 dB, protected again") {
            REQUIRE(reports.size() == count);
            REQUIRE(name_of(reports[last + 1][2]) == "arming");
            REQUIRE(value(reports[last + 1][3]) == Approx(-30.0).margin(1e-9));
            REQUIRE(value(reports[last + 1][4]) == 0.0);
        }
    }
}

SCENARIO("mutap.afc~'s guard ducks on a trip, its gain outlet carries the duck, and clear drops the strikes") {
    ext_main(nullptr);

    GIVEN("@kalman 1 @guard 1 @cap -12: 11 s, a 1 kHz near-end tone at 0 dBFS peak for 0.5 s, 2 s, clear, 1 s") {
        const long           quiet   = seconds_of(11.0);
        const long           loud    = seconds_of(0.5);
        const long           settle  = seconds_of(2.0);
        const long           tail    = seconds_of(1.0);
        const size_t         count   = reports_in(quiet + loud + settle + tail);
        static const run_log k_log   = run_logged(count, count, [=](mutap_afc& my_object, room& the_room) {
            my_object.kalman = true;
            my_object.guard  = true;
            my_object.cap    = atoms{-12.0};
            the_room.drive(my_object, quiet);
            the_room.tone = 1.0; // -3 dBFS RMS on the residual: over the detector's -6 dB ceiling
            the_room.drive(my_object, loud);
            the_room.tone = 0.0;
            the_room.drive(my_object, settle);
            my_object.clear();
            the_room.drive(my_object, tail);
        });
        const auto           reports = guard_reports(k_log.reports);

        THEN("the trip in probation ducks 20 dB under the cap with one strike") {
            REQUIRE(reports.size() == count);
            const auto ducked =
                std::find_if(reports.begin(), reports.end(), [](const auto& r) { return name_of(r[2]) == "ducked"; });
            REQUIRE(ducked != reports.end());
            const auto k = static_cast<size_t>(ducked - reports.begin());
            // Measured: report 236, 11.0063 s (the tone starts at 10.9990 s),
            // -32 dB (the cap, -12, less @duck 20), 1 strike.
            REQUIRE(report_time(k, k_mock_rate) >= static_cast<double>(quiet) / k_mock_rate);
            REQUIRE(value((*ducked)[3]) == Approx(-32.0).margin(1e-9));
            REQUIRE(value((*ducked)[5]) == 1.0);
        }
        THEN("the gain outlet is 1 until the tone, then reaches the duck relative to the cap, -20 dB") {
            const auto onset = static_cast<size_t>(quiet);
            REQUIRE(k_log.gain.size() == static_cast<size_t>(quiet + loud + settle + tail));
            REQUIRE(std::all_of(k_log.gain.begin(), k_log.gain.begin() + static_cast<std::ptrdiff_t>(onset),
                                [](double g) { return g == 1.0; }));
            const auto lowest = std::min_element(k_log.gain.begin(), k_log.gain.end());
            // Measured: 0.099999999999999978 at 11.0112 s (the bus stage's gain is
            // the mic's gain over its restore level, so 0 dB outside a duck).
            REQUIRE(*lowest == Approx(db_to_gain(-20.0)).margin(1e-12));
        }
        THEN("clear takes the strike back") {
            REQUIRE(reports.size() == count);
            REQUIRE(value(reports.back()[5]) == 0.0);
        }
    }
}

SCENARIO("mutap.afc~'s calibrate runs the soundcheck, reports calibrate_done, and the guard runs on it") {
    ext_main(nullptr);

    GIVEN("@kalman 1 @guard 1 @cal_s 1: 2 s, calibrate, 4 s, calibrate, 0.25 s, calibrate 0, 0.25 s") {
        const long           lead    = seconds_of(2.0);
        const long           open    = seconds_of(4.0);
        const long           quarter = seconds_of(0.25);
        const size_t         count   = reports_in(lead + open + 2 * quarter);
        static const run_log k_log   = run_logged(count + 2, count, [=](mutap_afc& my_object, room& the_room) {
            my_object.kalman = true;
            my_object.guard  = true;
            my_object.cal_s  = 1.0;
            the_room.drive(my_object, lead);
            my_object.calibrate();
            the_room.drive(my_object, open);
            my_object.calibrate();
            the_room.drive(my_object, quarter);
            my_object.calibrate(atoms{0});
            the_room.drive(my_object, quarter);
        });
        const auto           reports = guard_reports(k_log.reports);
        const auto           done    = calibrations(k_log.reports);

        THEN("two calibrate_done messages arrive: the 1 s window's and the one ended early") {
            // Measured: calibrate_done 3.65 -38.55, then 3.55 -38.65.
            REQUIRE(done.size() == 2);
            for (const auto& d : done) {
                REQUIRE(d.size() == 3);
                REQUIRE(std::isfinite(value(d[1])));
                REQUIRE(std::isfinite(value(d[2])));
            }
        }
        THEN("the first is median + margin: 4 dB over a shadow ratio and 3 dB over an uncertainty the reports saw") {
            REQUIRE(done.size() == 2);
            // The reports sample every 8th block of the window the sampler saw
            // whole: their range brackets its median.
            const size_t first = reports_in(lead);
            const size_t end   = reports_in(lead + seconds_of(1.0));
            double       d_lo = 1e9, d_hi = -1e9, a_lo = 1e9, a_hi = -1e9;
            for (size_t k = first; k < end; ++k) {
                d_lo = std::min(d_lo, value(reports[k][1]));
                d_hi = std::max(d_hi, value(reports[k][1]));
                a_lo = std::min(a_lo, value(reports[k][0]));
                a_hi = std::max(a_hi, value(reports[k][0]));
            }
            // Measured: D [-0.7915, -0.2316] and A' [-41.9841, -41.2638] dB in the
            // reports; median + margin came out 3.65 and -38.55. The 0.1 dB slack
            // is the sampler's bin width.
            REQUIRE(value(done[0][1]) - 4.0 >= d_lo - 0.1);
            REQUIRE(value(done[0][1]) - 4.0 <= d_hi + 0.1);
            REQUIRE(value(done[0][2]) - 3.0 >= a_lo - 0.1);
            REQUIRE(value(done[0][2]) - 3.0 <= a_hi + 0.1);
        }
        THEN("on the calibrated thresholds the verdict declares and the guard opens to 0 dB") {
            REQUIRE(reports.size() == count);
            const auto opened =
                std::find_if(reports.begin(), reports.end(), [](const auto& r) { return name_of(r[2]) == "open"; });
            REQUIRE(opened != reports.end());
            const auto   k             = static_cast<size_t>(opened - reports.begin());
            const size_t before_second = reports_in(lead + open) - 1;
            // Measured: open at report 96 (4.5047 s; calibrate_done at 3.02 s, then
            // the 1.5 s hold), 0 dB at report 128.
            REQUIRE(k < before_second);
            REQUIRE(name_of(reports[before_second][2]) == "open");
            REQUIRE(value(reports[before_second][3]) == Approx(0.0).margin(1e-9));
        }
    }
    GIVEN("a default instance (no guard), told to calibrate") {
        const auto run = run_logged(8, 8, [](mutap_afc& my_object, room& the_room) {
            my_object.calibrate();
            the_room.drive(my_object, 64 * 256);
        });
        THEN("nothing but the two-element reports arrives") {
            REQUIRE(run.reports.size() == 8);
            REQUIRE(calibrations(run.reports).empty());
        }
    }
}

SCENARIO("mutap.afc~ hands the guard's policy to the running guard at the next vector") {
    ext_main(nullptr);

    GIVEN("@kalman 1 @guard 1: 1 s, then @arming 20; and with @d_db 10 @a_db -30 from the start") {
        const long           second       = seconds_of(1.0);
        const size_t         count        = reports_in(2 * second);
        static const run_log k_arming_log = run_logged(count, count, [=](mutap_afc& my_object, room& the_room) {
            my_object.kalman = true;
            my_object.guard  = true;
            the_room.drive(my_object, second);
            my_object.arming = 20.0;
            the_room.drive(my_object, second);
        });
        const size_t         long_count   = reports_in(seconds_of(3.0));
        static const run_log k_thresholds_log =
            run_logged(long_count, long_count, [](mutap_afc& my_object, room& the_room) {
                my_object.kalman = true;
                my_object.guard  = true;
                my_object.d_db   = 10.0;
                my_object.a_db   = -30.0;
                the_room.drive(my_object, seconds_of(3.0));
            });

        THEN("ARMING moves from -30 to -20 dB") {
            const auto reports = guard_reports(k_arming_log.reports);
            REQUIRE(reports.size() == count);
            const size_t change = reports_in(second);
            REQUIRE(value(reports[change - 1][3]) == Approx(-30.0).margin(1e-9));
            // Measured: -20 dB at the last report (the 200 ms ramp up is long done).
            REQUIRE(name_of(reports.back()[2]) == "arming");
            REQUIRE(value(reports.back()[3]) == Approx(-20.0).margin(1e-9));
        }
        THEN("thresholds the room's statistics meet let the verdict declare") {
            const auto reports = guard_reports(k_thresholds_log.reports);
            REQUIRE(reports.size() == long_count);
            const auto opened =
                std::find_if(reports.begin(), reports.end(), [](const auto& r) { return name_of(r[2]) == "open"; });
            REQUIRE(opened != reports.end());
            const auto k = static_cast<size_t>(opened - reports.begin());
            // Measured: open at report 45 (2.1362 s), 0 dB at the last.
            REQUIRE(report_time(k, k_mock_rate) < 3.0);
            REQUIRE(value(reports.back()[3]) == Approx(0.0).margin(1e-9));
        }
    }
}

SCENARIO("mutap.afc~'s guard survives a sample-rate rebuild") {
    ext_main(nullptr);

    GIVEN("@kalman 1 @guard 1 at 44.1 kHz, then dspsetup at 96 kHz") {
        const auto run = run_logged(16, 16, [](mutap_afc& my_object, room& the_room) {
            my_object.kalman = true;
            my_object.guard  = true;
            the_room.drive(my_object, 64 * 256);
            my_object.samplerate(96000.0);
            my_object.dspsetup(atoms{96000.0, k_vector_size});
            the_room.drive(my_object, 64 * 256);
        });
        THEN("the rebuilt engine carries a guard, which reports from ARMING again") {
            const auto reports = guard_reports(run.reports);
            REQUIRE(reports.size() == 16);
            REQUIRE(name_of(reports[8][2]) == "arming");
            REQUIRE(value(reports[8][3]) == Approx(-30.0).margin(1e-9));
        }
    }
}
