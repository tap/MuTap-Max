/// @file
/// @brief      Unit tests for mutap.aec~ (Min-level: attribute defaults, rounding, clamping).
// SPDX-License-Identifier: MIT
// Copyright 2026 MuTap contributors

#include "c74_min_unittest.h"    // required unit-test header (defines main via Catch)
#include "mutap.aec_tilde.cpp"   // include the object source so we can instantiate it

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
            REQUIRE(static_cast<bool>(my_object.postfilter) == false);
            REQUIRE(static_cast<bool>(my_object.comfort) == true);
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
