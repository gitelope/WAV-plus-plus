cmake_minimum_required (VERSION 3.20)

project (wav_plus_plus
    VERSION 0.1.0
    LANGUAGES CXX
)
add_library (wav_plus_plus INTERFACE)
target_include_directories (wav_plus_plus INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# C++20
target_compile_features (wav_plus_plus INTERFACE cxx_std_20)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# ---- Library Headers ----
#          Public
set (wav_plus_plus_public_headers
    include/wav-plus-plus/ChunkInfo.hpp
    include/wav-plus-plus/Exceptions.hpp
    include/wav-plus-plus/FileInfo.hpp
    include/wav-plus-plus/Formats.hpp
    include/wav-plus-plus/Reader.hpp
    include/wav-plus-plus/Writer.hpp
)
#          Private
set (wav_plus_plus_private_headers
    include/wav-plus-plus/private/AudioSummary.hpp
    include/wav-plus-plus/private/BufferSizing.hpp
    include/wav-plus-plus/private/Byteswap.hpp
    include/wav-plus-plus/private/ChunkInfoImplementation.hpp
    include/wav-plus-plus/private/FileInfoImplementation.hpp
    include/wav-plus-plus/private/FormatsImplementation.hpp
    include/wav-plus-plus/private/QuantizationConstants.hpp
    include/wav-plus-plus/private/ReaderImplementation.hpp
    include/wav-plus-plus/private/ReadingBytes.hpp
    include/wav-plus-plus/private/WriterImplementation.hpp
    include/wav-plus-plus/private/int24.hpp
)
#          All
set (wav_plus_plus_all_headers
    ${wav_plus_plus_public_headers}
    ${wav_plus_plus_private_headers}
)
source_group (TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${wav_plus_plus_all_headers})
# IDE-only target so include/ appears once in navigator
add_custom_target (wav_plus_plus_headers SOURCES ${wav_plus_plus_all_headers})


# ---- Tests ----
add_executable (tests    tests/tests.cpp)

set (wav_plus_plus_test_headers
    tests/TestLog.hpp
    tests/helper_functions.hpp

    tests/reading/ReadingTests.hpp
    tests/reading/decoding/SampleDecoding.hpp
    tests/reading/headers/HeaderParsing.hpp
    tests/reading/headers/PCMWAVEFORMAT.hpp
    tests/reading/headers/WAVEFORMAT.hpp
    tests/reading/headers/WAVEFORMATEX.hpp
    tests/reading/headers/WAVEFORMATEXTENSIBLE.hpp
    tests/reading/headers/header_parsing_helper_functions.hpp
    tests/reading/reading_helper_functions.hpp

    tests/utilities/Byteswap.hpp
    tests/utilities/UtilitiesTests.hpp

    tests/writing/HeaderWriting.hpp
    tests/writing/SampleEncoding.hpp
    tests/writing/WritingTests.hpp
)
source_group (TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${wav_plus_plus_test_headers})
target_sources (tests PRIVATE ${wav_plus_plus_test_headers})
target_link_libraries (tests PRIVATE wav_plus_plus)
enable_testing()
add_test(NAME tests COMMAND tests)


# ---- Examples ----

add_executable (example_read       examples/read.cpp)
target_link_libraries (example_read PRIVATE wav_plus_plus)

add_executable (example_write      examples/write.cpp)
target_link_libraries (example_write PRIVATE wav_plus_plus)

add_executable (example_inspect    examples/inspect.cpp)
target_link_libraries (example_inspect PRIVATE wav_plus_plus)
