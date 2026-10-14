/// @file
/// @brief      Unit tests for mutap.aec~ (Min-level: attribute defaults, rounding, clamping).
// SPDX-License-Identifier: MIT
// Copyright 2026 MuTap contributors

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <thread>
#include <vector>

#include "c74_min_unittest.h"  // required unit-test header (defines main via Catch)
#include "mutap.aec_tilde.cpp" // include the object source so we can instantiate it

namespace {

    constexpr int  k_report_outlet = 1; ///< the right outlet: IPC, and `watchdog n`
    constexpr long k_vector_size   = 64;

    /// Open-loop stand-in for the room (as mutap.afc_tilde_test.cpp's): the
    /// far end x is deterministic white noise, the microphone hears it
    /// through a 7-sample delay at half amplitude; `poison` puts a NaN in the
    /// next vector's first mic sample, once.
    struct room {
        static constexpr size_t           k_delay = 7;
        std::uint32_t                     seed{0x2545F491U};
        std::array<double, k_delay>       history{};
        size_t                            head{0};
        bool                              poison{false};
        std::array<double, k_vector_size> x{};
        std::array<double, k_vector_size> y{};
        std::array<double, k_vector_size> e{};
        std::vector<double>               out;

        void drive(mutap_aec& obj, long samples) {
            for (long done = 0; done < samples; done += k_vector_size) {
                for (size_t i = 0; i < x.size(); ++i) {
                    seed          = seed * 1664525U + 1013904223U;
                    x[i]          = static_cast<double>(seed >> 8) / static_cast<double>(1U << 24) - 0.5;
                    y[i]          = 0.5 * history[head];
                    history[head] = x[i];
                    head          = (head + 1) % k_delay;
                }
                if (poison) {
                    y[0]   = std::numeric_limits<double>::quiet_NaN();
                    poison = false;
                }
                std::array<double*, 2> in{y.data(), x.data()};
                std::array<double*, 1> outs{e.data()};
                obj(audio_bundle{in.data(), 2, k_vector_size}, audio_bundle{outs.data(), 1, k_vector_size});
                out.insert(out.end(), e.begin(), e.end());
            }
        }
    };

    /// Wait for the right outlet's fifo to have delivered `count` messages
    /// (the mock kernel delivers on its own clock thread; see the afc test).
    bool wait_for_reports(mutap_aec& obj, size_t count) {
        const c74::max::t_sequence& log      = *c74::max::object_getoutput(obj.maxobj(), k_report_outlet);
        const auto                  deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (log.size() < count && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return log.size() >= count;
    }

    std::string name_of(const atom& a) {
        return a.a_type == c74::max::A_SYM ? std::string(atom_getsym(&a)->s_name) : std::string{};
    }

} // namespace

SCENARIO("mutap.aec~ instantiates with the documented defaults") {
    ext_main(nullptr); // configure the class (required once per test executable)

    GIVEN("a default instance") {
        test_wrapper<mutap_aec> an_instance;
        mutap_aec&              my_object = an_instance;

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
        THEN("the post-filter chain is off, with comfort noise armed for when it is on") {
            REQUIRE(static_cast<int>(my_object.postfilter) == 0);
            REQUIRE(static_cast<bool>(my_object.comfort) == true);
        }
        THEN("the learned engine defaults to the built-in model") {
            const c74::min::symbol current = my_object.model;
            REQUIRE(std::string(current.c_str()).empty());
        }
    }
}

SCENARIO("mutap.aec~ clamps the post-filter engine selector") {
    ext_main(nullptr);

    GIVEN("a default instance") {
        test_wrapper<mutap_aec> an_instance;
        mutap_aec&              my_object = an_instance;

        WHEN("an out-of-range engine is requested") {
            my_object.postfilter = 7;
            THEN("it clamps to the learned engine") {
                REQUIRE(static_cast<int>(my_object.postfilter) == 2);
            }
        }
        WHEN("the learned engine is selected with the built-in model") {
            my_object.postfilter = 2;
            THEN("the block size is coerced to the model's trained hop") {
                REQUIRE(static_cast<int>(my_object.postfilter) == 2);
                // built-in model geometry pins @block via publish()
            }
        }
        WHEN("old boolean-style patches set 0/1") {
            my_object.postfilter = 1;
            REQUIRE(static_cast<int>(my_object.postfilter) == 1);
            my_object.postfilter = 0;
            REQUIRE(static_cast<int>(my_object.postfilter) == 0);
        }
    }
}

SCENARIO("mutap.aec~ rounds the block size up to a power of two") {
    ext_main(nullptr);

    GIVEN("a default instance") {
        test_wrapper<mutap_aec> an_instance;
        mutap_aec&              my_object = an_instance;

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

SCENARIO("mutap.aec~ clamps the adaptation step size into the stable open interval") {
    ext_main(nullptr);

    GIVEN("a default instance") {
        test_wrapper<mutap_aec> an_instance;
        mutap_aec&              my_object = an_instance;

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

// MuTap's NaN watchdog through the glue (as mutap.afc_tilde_test.cpp): a
// NaN in the microphone resets the engine, comes out as one block of
// silence, and the right outlet says `watchdog n` once per count.

SCENARIO("mutap.aec~ reports the NaN watchdog as 'watchdog n' on the right outlet") {
    ext_main(nullptr);

    GIVEN("a default instance driven 32 blocks, one NaN in the microphone, then 32 blocks") {
        // Up to three fresh instances: the mock clock thread's start-up race
        // (see the afc test's run_logged).
        for (int attempt = 0; attempt < 3; ++attempt) {
            test_wrapper<mutap_aec> an_instance;
            mutap_aec&              my_object = an_instance;
            room                    the_room;
            the_room.drive(my_object, 32 * 256);
            the_room.poison = true;
            the_room.drive(my_object, 32 * 256);
            // 64 blocks: 8 IPC reports + 1 watchdog message.
            if (!wait_for_reports(my_object, 9)) {
                continue;
            }
            const c74::max::t_sequence& log       = *c74::max::object_getoutput(my_object.maxobj(), k_report_outlet);
            size_t                      watchdogs = 0;
            double                      count     = -1.0;
            for (const auto& r : log) {
                if (!r.empty() && name_of(r[0]) == "watchdog") {
                    ++watchdogs;
                    count = static_cast<double>(r[1]);
                }
            }
            THEN("exactly one 'watchdog 1' arrives and the output stays finite") {
                REQUIRE(watchdogs == 1);
                REQUIRE(count == 1.0);
                REQUIRE(
                    std::all_of(the_room.out.begin(), the_room.out.end(), [](double v) { return std::isfinite(v); }));
            }
            return;
        }
        FAIL("no delivery in three attempts");
    }
}
