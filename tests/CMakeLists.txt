cmake_minimum_required(VERSION 3.13)

# Standalone NATIVE test project. Do not configure this via the firmware's ARM
# toolchain - the tests are meant to be compiled and run on the host machine:
#
#   cmake -S tests -B build-tests
#   cmake --build build-tests
#   ctest --test-dir build-tests --output-on-failure
#
# Build/run a single test:
#   cmake --build build-tests --target template_test
#   ./build-tests/template_test
project(RS41ng_tests C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src)

# The firmware requires a hardware type to be defined. Default to RS41 for the
# host tests; override with -DDFM17=1 or -DRS41_RSM4X4=1 when configuring.
if(NOT DEFINED RS41 AND NOT DEFINED DFM17 AND NOT DEFINED RS41_RSM4X4)
    set(RS41 1)
endif()
foreach(hw RS41 RS41_RSM4X4 DFM17)
    if(DEFINED ${hw} AND ${hw})
        add_compile_definitions(${hw}=1)
    endif()
endforeach()

# On macOS the libc <string.h> defines a fortifying strlcpy(...) macro that
# clashes with the project's own strlcpy declaration. Disable fortification so
# the host build matches the firmware's plain libc behaviour.
if(APPLE)
    add_compile_definitions(_FORTIFY_SOURCE=0)
endif()

include_directories(${SRC_DIR})

# Host-compilable firmware sources shared by the tests (no HAL / hardware deps).
set(COMMON_SOURCES
        ${SRC_DIR}/config.c
        ${SRC_DIR}/template.c
        ${SRC_DIR}/utils.c
        ${SRC_DIR}/strlcpy.c
        ${SRC_DIR}/codecs/aprs/aprs.c
        ${SRC_DIR}/codecs/aprs/aprs_position.c
        ${SRC_DIR}/codecs/ax25/ax25.c
        ${SRC_DIR}/codecs/bell/bell.c
        ${SRC_DIR}/codecs/morse/morse.c
)

enable_testing()

# One executable + one ctest per *_test.c file that defines a real entry point.
# Tests can be parked by renaming main() (e.g. bell_test.c uses main2()); those
# are skipped automatically.
file(GLOB TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*_test.c)
foreach(test_src ${TEST_SOURCES})
    file(READ ${test_src} test_contents)
    if(test_contents MATCHES "int[ \t\r\n]+main[ \t]*\\(")
        get_filename_component(test_name ${test_src} NAME_WE)
        add_executable(${test_name} ${test_src} ${COMMON_SOURCES})
        add_test(NAME ${test_name} COMMAND ${test_name})
    else()
        get_filename_component(test_name ${test_src} NAME_WE)
        message(STATUS "Skipping ${test_name}: no main() entry point")
    endif()
endforeach()
