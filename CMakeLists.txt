cmake_minimum_required(VERSION 3.20)

project(wav_plus_plus
    VERSION 0.1.0
    LANGUAGES CXX
)

add_library(wav_plus_plus INTERFACE)

target_include_directories(wav_plus_plus INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_features(wav_plus_plus INTERFACE cxx_std_20)

add_executable(tests
    tests/tests.cpp
)

target_link_libraries(tests PRIVATE
    wav_plus_plus
)

enable_testing()

add_test(NAME tests COMMAND tests)

add_executable(example_write
    examples/write.cpp
)

target_link_libraries(example_write PRIVATE
    wav_plus_plus
)

add_executable(example_read
    examples/read.cpp
)

target_link_libraries(example_read PRIVATE
    wav_plus_plus
)

add_executable(example_inspect
    examples/inspect.cpp
)

target_link_libraries(example_inspect PRIVATE
    wav_plus_plus
)
