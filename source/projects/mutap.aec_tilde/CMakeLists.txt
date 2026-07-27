# mutap.aec~ — acoustic echo canceller.

cmake_minimum_required(VERSION 3.19)

set(C74_MIN_API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../min-api)
include(${C74_MIN_API_DIR}/script/min-pretarget.cmake)

include_directories(
    "${C74_INCLUDES}"
)

set(SOURCE_FILES
    ${PROJECT_NAME}.cpp
)

add_library(
    ${PROJECT_NAME}
    MODULE
    ${SOURCE_FILES}
)

# MuTap headers (header-only pem_afc/fdaf/lpc) + tap::dsp (the vendored
# Ooura rdft that tap::mu::basic_real_fft calls); the `mutap` interface target
# carries both.
target_link_libraries(${PROJECT_NAME} PRIVATE mutap)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

include(${C74_MIN_API_DIR}/script/min-posttarget.cmake)

# Unit test (min-api's Catch2 harness), if the test file exists.
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}_test.cpp")
    include(${C74_MIN_API_DIR}/test/min-object-unittest.cmake)
    # The harness builds a SEPARATE ${PROJECT_NAME}_test executable, which does not
    # inherit the target-scoped libraries given to the external above. It needs the
    # same `mutap` interface target for the kernel headers and tap::dsp.
    target_link_libraries(${PROJECT_NAME}_test PRIVATE mutap)
endif ()
