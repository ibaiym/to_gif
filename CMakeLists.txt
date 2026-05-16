cmake_minimum_required(VERSION 3.16)
project(to_gif_optimizer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find dependencies
find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBAVCODEC REQUIRED libavcodec)
pkg_check_modules(LIBAVFORMAT REQUIRED libavformat)
pkg_check_modules(LIBSWSCALE REQUIRED libswscale)
pkg_check_modules(LIBAVUTIL REQUIRED libavutil)
pkg_check_modules(LIBIMAGEQUANT REQUIRED imagequant)

# giflib doesn't have a pkg-config file, search manually
find_library(GIF_LIBRARY gif
    PATHS /usr/lib/x86_64-linux-gnu /usr/lib /usr/local/lib
          /usr/lib/aarch64-linux-gnu /usr/lib/arm-linux-gnueabihf)
find_path(GIF_INCLUDE_DIR gif_lib.h
    PATHS /usr/include /usr/local/include)
if(NOT GIF_LIBRARY OR NOT GIF_INCLUDE_DIR)
    message(FATAL_ERROR "Could not find giflib")
endif()

# ------------------------------------------------------------------
# Library (shared by main executable and tests)
# ------------------------------------------------------------------

set(LIB_SOURCES
    src/common/types.cpp
    src/components/ffmpeg_decoder.cpp
    src/components/imagequant_quantizer.cpp
    src/components/giflib_encoder.cpp
    src/factories/default_pipeline_factory.cpp
    src/optimizer/predictive_search.cpp
    src/optimizer/parameter_optimizer.cpp
    src/pipeline/gif_pipeline.cpp
    src/utils/file_utils.cpp
    src/utils/logger.cpp
    src/utils/math_utils.cpp
)

add_library(to_gif_lib STATIC ${LIB_SOURCES})

target_include_directories(to_gif_lib PUBLIC
    ${LIBAVCODEC_INCLUDE_DIRS}
    ${LIBAVFORMAT_INCLUDE_DIRS}
    ${LIBSWSCALE_INCLUDE_DIRS}
    ${LIBAVUTIL_INCLUDE_DIRS}
    ${LIBIMAGEQUANT_INCLUDE_DIRS}
    ${GIF_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(to_gif_lib PUBLIC
    ${LIBAVCODEC_LIBRARIES}
    ${LIBAVFORMAT_LIBRARIES}
    ${LIBSWSCALE_LIBRARIES}
    ${LIBAVUTIL_LIBRARIES}
    ${LIBIMAGEQUANT_LIBRARIES}
    ${GIF_LIBRARY}
)

target_compile_options(to_gif_lib PUBLIC
    ${LIBAVCODEC_CFLAGS_OTHER}
    ${LIBAVFORMAT_CFLAGS_OTHER}
    ${LIBSWSCALE_CFLAGS_OTHER}
    ${LIBAVUTIL_CFLAGS_OTHER}
    ${LIBIMAGEQUANT_CFLAGS_OTHER}
)
target_compile_options(to_gif_lib PRIVATE -Wall -Wextra)

# Release mode optimization
target_compile_options(to_gif_lib PRIVATE $<$<CONFIG:Release>:-O3>)

# ------------------------------------------------------------------
# Main executable
# ------------------------------------------------------------------

add_executable(to_gif src/main.cpp)
target_link_libraries(to_gif PRIVATE to_gif_lib)

# ------------------------------------------------------------------
# Tests (GoogleTest via system package)
# ------------------------------------------------------------------

find_package(GTest)
if(NOT GTest_FOUND)
    find_library(GTEST_LIBRARY gtest PATHS /usr/lib/x86_64-linux-gnu)
    find_library(GTEST_MAIN_LIBRARY gtest_main PATHS /usr/lib/x86_64-linux-gnu)
    find_path(GTEST_INCLUDE_DIR gtest/gtest.h PATHS /usr/include)
    if(GTEST_LIBRARY AND GTEST_MAIN_LIBRARY AND GTEST_INCLUDE_DIR)
        add_library(GTest::gtest UNKNOWN IMPORTED)
        set_target_properties(GTest::gtest PROPERTIES IMPORTED_LOCATION ${GTEST_LIBRARY} INTERFACE_INCLUDE_DIRECTORIES ${GTEST_INCLUDE_DIR})
        add_library(GTest::gtest_main UNKNOWN IMPORTED)
        set_target_properties(GTest::gtest_main PROPERTIES IMPORTED_LOCATION ${GTEST_MAIN_LIBRARY} INTERFACE_INCLUDE_DIRECTORIES ${GTEST_INCLUDE_DIR})
    else()
        message(FATAL_ERROR "Could not find googletest. Please install libgtest-dev.")
    endif()
endif()

enable_testing()

set(TEST_SOURCES
    tests/test_predictive_search.cpp
    tests/test_logger.cpp
    tests/test_file_utils.cpp
    tests/test_gif_pipeline.cpp
    tests/test_parameter_optimizer.cpp
    tests/test_config_validate.cpp
    tests/test_math_utils.cpp
)

add_executable(to_gif_tests ${TEST_SOURCES})
target_link_libraries(to_gif_tests PRIVATE to_gif_lib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(to_gif_tests)
