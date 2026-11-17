# WAV++
# Copyright (c) 2025 David Adams
# SPDX-License-Identifier: MIT

add_executable (tests tests.cpp)

# Listed only so the headers appear in IDE projects: tests.cpp reaches them
# all through the umbrella headers, so nothing compiles from this list.
file (GLOB_RECURSE wav_plus_plus_test_headers CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

source_group (TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${wav_plus_plus_test_headers})

target_sources (tests PRIVATE ${wav_plus_plus_test_headers})

target_link_libraries (tests PRIVATE wav_plus_plus)

target_compile_options (tests PRIVATE ${wav_plus_plus_warning_flags})

set_target_properties (tests PROPERTIES CXX_EXTENSIONS OFF)

add_test (NAME tests COMMAND tests)
