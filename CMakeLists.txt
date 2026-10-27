cmake_minimum_required(VERSION 3.20)
project(moving2arm LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)

option(MOVING2ARM_BUILD_LABS "Build the additional portability labs" ON)

option(MOVING2ARM_ENABLE_SVE "Build SVE examples; execution requires SVE hardware or emulation" OFF)
option(MOVING2ARM_ENABLE_SVE2 "Build SVE2 example; execution requires SVE2 hardware or emulation" OFF)
set(MOVING2ARM_SVE_FLAGS "-march=armv8-a+sve" CACHE STRING "Compiler flags for SVE targets")
set(MOVING2ARM_SVE2_FLAGS "-march=armv8-a+sve2" CACHE STRING "Compiler flags for SVE2 targets")

function(configure_example target)
    target_include_directories(${target} PRIVATE "${PROJECT_SOURCE_DIR}/simple_port")
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    if(UNIX)
        target_link_libraries(${target} PRIVATE m)
    endif()
endfunction()

set(simple_port_targets)
function(add_calculation name source)
    add_executable(${name} ${source})
    configure_example(${name})
    set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/simple_port")
    set(targets ${name})
    if(BUILD_TESTING)
        add_executable(${name}_test ${source} tests/test_calculation.c)
        target_compile_definitions(${name}_test PRIVATE CALCULATION_NO_MAIN)
        configure_example(${name}_test)
        set_target_properties(${name}_test PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/simple_port")
        list(APPEND targets ${name}_test)
        add_test(NAME ${name}.correctness COMMAND ${name}_test)
        # A CMake driver checks stdout and also preserves the emulator for cross
        # builds, where the demo cannot be executed directly by the host.
        set(demo_emulator)
        if(CMAKE_CROSSCOMPILING)
            get_target_property(demo_emulator ${name} CROSSCOMPILING_EMULATOR)
            if(NOT demo_emulator)
                set(demo_emulator)
            endif()
        endif()
        add_test(NAME ${name}.demo COMMAND "${CMAKE_COMMAND}"
            "-DPROGRAM=$<TARGET_FILE:${name}>" "-DEMULATOR=${demo_emulator}"
            -P "${PROJECT_SOURCE_DIR}/tests/check_calculation_demo.cmake")
        set_tests_properties(${name}.correctness PROPERTIES
            LABELS "simple_port;correctness" TIMEOUT 30)
        set_tests_properties(${name}.demo PROPERTIES
            LABELS "simple_port;demo" TIMEOUT 30)
    endif()
    set(simple_port_targets ${simple_port_targets} ${targets} PARENT_SCOPE)
endfunction()

function(calculation_include name directory)
    target_include_directories(${name} SYSTEM PRIVATE "${directory}")
    if(BUILD_TESTING)
        target_include_directories(${name}_test SYSTEM PRIVATE "${directory}")
    endif()
endfunction()

function(calculation_flags name flags)
    separate_arguments(options NATIVE_COMMAND "${flags}")
    target_compile_options(${name} PRIVATE ${options})
    if(BUILD_TESTING)
        target_compile_options(${name}_test PRIVATE ${options})
    endif()
endfunction()

add_calculation(calculation simple_port/calculation.c)

check_c_source_compiles("#include <xmmintrin.h>
int main(void) { __m128 a = _mm_set1_ps(4.0f); return _mm_cvtss_f32(_mm_sqrt_ps(a)) != 2.0f; }"
    HAVE_SSE)
if(HAVE_SSE)
    add_calculation(calculation_sse simple_port/calculation_sse.c)
else()
    message(STATUS "Skipping SSE: compiler target doesn't support x86 SSE")
endif()

check_c_source_compiles("#include <arm_neon.h>
int main(void) { float32x4_t a = vdupq_n_f32(4.0f); return vgetq_lane_f32(vsqrtq_f32(a), 0) != 2.0f; }"
    HAVE_AARCH64_NEON)
if(HAVE_AARCH64_NEON)
    add_calculation(calculation_neon simple_port/calculation_neon.c)
else()
    message(STATUS "Skipping NEON: compiler target doesn't support AArch64 NEON")
endif()

find_path(SIMDE_INCLUDE_DIR simde/x86/sse.h
    HINTS "${PROJECT_SOURCE_DIR}/multi-platform-dev/simde")
if(SIMDE_INCLUDE_DIR)
    foreach(variant quick better)
        add_calculation(calculation_simde_${variant} simple_port/calculation_simde_${variant}.c)
        calculation_include(calculation_simde_${variant} "${SIMDE_INCLUDE_DIR}")
    endforeach()
else()
    message(STATUS "Skipping SIMDe: initialize submodules or set SIMDE_INCLUDE_DIR")
endif()

find_path(XSIMD_INCLUDE_DIR xsimd/xsimd.hpp)
if(XSIMD_INCLUDE_DIR)
    set(CMAKE_REQUIRED_INCLUDES "${XSIMD_INCLUDE_DIR}")
    check_cxx_source_compiles("#include <xsimd/xsimd.hpp>
int main() { using b = xsimd::batch<float>; float v[b::size] = {}; b::load_unaligned(v).store_unaligned(v); }"
        HAVE_XSIMD)
    unset(CMAKE_REQUIRED_INCLUDES)
    if(NOT HAVE_XSIMD)
        message(FATAL_ERROR "xsimd headers were found, but batch load/store APIs failed to compile. See CMake configure diagnostics.")
    endif()
    add_calculation(calculation_xsimd simple_port/calculation_xsimd.cpp)
    calculation_include(calculation_xsimd "${XSIMD_INCLUDE_DIR}")
else()
    message(STATUS "Skipping xsimd: provide xsimd 8+ headers with XSIMD_INCLUDE_DIR")
endif()

if(MOVING2ARM_ENABLE_SVE)
    set(CMAKE_REQUIRED_FLAGS "${MOVING2ARM_SVE_FLAGS}")
    check_c_source_compiles("#include <arm_sve.h>
int main(void) { return (int)svcntw(); }" HAVE_SVE)
    unset(CMAKE_REQUIRED_FLAGS)
    if(NOT HAVE_SVE)
        message(FATAL_ERROR "SVE requested but unavailable for this compiler target. Use an AArch64 Linux compiler and MOVING2ARM_SVE_FLAGS.")
    endif()
    add_calculation(calculation_sve simple_port/calculation_sve.c)
    calculation_flags(calculation_sve "${MOVING2ARM_SVE_FLAGS}")
    if(BUILD_TESTING)
        target_compile_definitions(calculation_sve_test PRIVATE CALCULATION_TEST_SVE)
    endif()
endif()

add_custom_target(simple_port_examples DEPENDS ${simple_port_targets})
if(BUILD_TESTING)
    add_custom_target(check_simple_port
        COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${PROJECT_BINARY_DIR}"
            --output-on-failure -L simple_port -C "$<CONFIG>"
        DEPENDS ${simple_port_targets}
        USES_TERMINAL VERBATIM)
endif()

if(MOVING2ARM_ENABLE_SVE2)
    set(CMAKE_REQUIRED_FLAGS "${MOVING2ARM_SVE2_FLAGS}")
    check_c_source_compiles("#include <arm_sve.h>
int main(void) { svint32_t a = svdup_s32(1); return svlastb_s32(svptrue_b32(), svqadd_s32_m(svptrue_b32(), a, a)); }"
        HAVE_SVE2)
    unset(CMAKE_REQUIRED_FLAGS)
    if(NOT HAVE_SVE2)
        message(FATAL_ERROR "SVE2 requested but unavailable for this compiler target. Use an AArch64 Linux compiler and MOVING2ARM_SVE2_FLAGS.")
    endif()
    add_executable(arm_sve2 arm_sve2/arm_sve2.c)
    configure_example(arm_sve2)
    separate_arguments(sve2_options NATIVE_COMMAND "${MOVING2ARM_SVE2_FLAGS}")
    target_compile_options(arm_sve2 PRIVATE ${sve2_options})
    if(BUILD_TESTING)
        add_test(NAME arm_sve2.correctness COMMAND arm_sve2)
    endif()
endif()

# Native Linux can select an inherited SVE vector length before exec. Cross
# builds instead select vector length in their emulator (see the CI workflow).
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING
        AND (MOVING2ARM_ENABLE_SVE OR MOVING2ARM_ENABLE_SVE2))
    add_executable(sve_vl_runner tests/sve_vl_runner.c)
    configure_example(sve_vl_runner)
    add_dependencies(check_simple_port sve_vl_runner)
    foreach(bits 128 256 512 2048)
        if(MOVING2ARM_ENABLE_SVE)
            add_test(NAME calculation_sve.vl${bits}
                COMMAND sve_vl_runner ${bits} $<TARGET_FILE:calculation_sve_test>)
            set_tests_properties(calculation_sve.vl${bits} PROPERTIES
                SKIP_RETURN_CODE 77 LABELS "simple_port;sve" TIMEOUT 30)
        endif()
        if(MOVING2ARM_ENABLE_SVE2)
            add_test(NAME arm_sve2.vl${bits}
                COMMAND sve_vl_runner ${bits} $<TARGET_FILE:arm_sve2>)
            set_tests_properties(arm_sve2.vl${bits} PROPERTIES SKIP_RETURN_CODE 77)
        endif()
    endforeach()
endif()

if(MOVING2ARM_BUILD_LABS)
    if(NOT UNIX)
        message(FATAL_ERROR "The additional labs require Linux or macOS. Set MOVING2ARM_BUILD_LABS=OFF for a SIMD-only build on other systems.")
    endif()
    add_subdirectory(porting_labs)
endif()
