/// @file
/// @brief      Unit tests for mutap.afc~ (Min-level: attribute defaults, rounding, clamping,
///             and the convergence outlet's structure).
// SPDX-License-Identifier: MIT
// Copyright 2026 MuTap contributors

#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <thread>

#include "c74_min_unittest.h"  // required unit-test header (defines main via Catch)
#include "mutap.afc_tilde.cpp" // include the object source so we can instantiate it

namespace {

    constexpr int  k_ipc_outlet  = 1;
    constexpr int  k_conv_outlet = 2;
    constexpr long k_vector_size = 64; ///< host vector size the tests drive (smaller than every @block used)

    /// Open-loop stand-in for the room: the reference u is deterministic white
    /// noise and the microphone hears it through a 7-sample delay at half
    /// amplitude, so every engine has a path to identify. The state carries
    /// across perform calls.
    struct room {
        static constexpr size_t k_delay = 7;

        std::uint32_t                     seed{0x2545F491U};
        std::array<double, k_delay>       history{};
        size_t                            head{0};
        std::array<double, k_vector_size> u{};
        std::array<double, k_vector_size> y{};
        std::array<double, k_vector_size> e{};

        /// Run `samples` samples (a multiple of k_vector_size) through the object's perform routine.
        void drive(mutap_afc& obj, long samples) {
            for (long done = 0; done < samples; done += k_vector_size) {
                for (size_t i = 0; i < u.size(); ++i) {
                    seed          = seed * 1664525U + 1013904223U;
                    u[i]          = static_cast<double>(seed >> 8) / static_cast<double>(1U << 24) - 0.5;
                    y[i]          = 0.5 * history[head];
                    history[head] = u[i];
                    head          = (head + 1) % k_delay;
                }
                std::array<double*, 2> in{y.data(), u.data()};
                std::array<double*, 1> out{e.data()};
                obj(audio_bundle{in.data(), 2, k_vector_size}, audio_bundle{out.data(), 1, k_vector_size});
            }
        }
    };

    /// The IPC and convergence outlets are fifo outlets bound to the scheduler
    /// thread; from the test's (main) thread they queue, and the mock kernel
    /// delivers each on its own clock thread. Wait until both outlets have
    /// logged `count` messages, then give the clock threads' remaining (empty)
    /// deliveries time to run before the object can be freed. The mock offers
    /// no synchronization with those threads, so this polls. False when the
    /// deadline passed first.
    bool wait_for_reports(mutap_afc& obj, size_t count) {
        const c74::max::t_sequence& conv     = *c74::max::object_getoutput(obj.maxobj(), k_conv_outlet);
        const c74::max::t_sequence& ipc      = *c74::max::object_getoutput(obj.maxobj(), k_ipc_outlet);
        const auto                  deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while ((conv.size() < count || ipc.size() < count) && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return conv.size() >= count && ipc.size() >= count;
    }

    /// Build a default instance, let `run` configure it and drive it through
    /// a room, and return the convergence outlet's log once `count` reports
    /// have arrived on both report outlets (empty if they never did). A fresh
    /// instance is tried up to three times: min-api's mock clock
    /// (test/mock/c74_mock_clock.h) starts its thread in a member initializer
    /// that precedes the flag the thread loops on, and a thread that wins
    /// that race exits at once and never delivers. Measured on the Intel Mac
    /// with 16 test binaries running at once: before this retry, 8 of 80
    /// binary runs failed on a missing delivery; with it, 36 first attempts
    /// failed over 160 binary runs, no second attempt did, and all 160
    /// passed. Run one at a time, 0 of 30 binary runs needed a retry. Max
    /// has no such thread; an external that never sends fails every attempt.
    template <typename Run>
    c74::max::t_sequence reports_from(size_t count, Run&& run) {
        for (int attempt = 0; attempt < 3; ++attempt) {
            test_wrapper<mutap_afc> an_instance;
            mutap_afc&              my_object = an_instance;
            room                    the_room;
            run(my_object, the_room);
            if (wait_for_reports(my_object, count)) {
                return *c74::max::object_getoutput(my_object.maxobj(), k_conv_outlet);
            }
        }
        return {};
    }

    double value(const atom& a) {
        return static_cast<double>(a);
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
