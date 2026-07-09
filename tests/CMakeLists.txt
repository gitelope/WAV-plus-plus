# WAV++ (c) 2025-2026 David Adams
# SPDX-License-Identifier: MIT

add_executable (tests tests.cpp)

set (wav_plus_plus_test_headers
    TestLog.hpp
    helper_functions.hpp

    reading/ReadingTests.hpp
    reading/reading_helper_functions.hpp
    
    reading/headers/HeaderParsing.hpp
    reading/headers/header_parsing_helper_functions.hpp

    reading/headers/formats/Formats.hpp
    reading/headers/formats/PCMWAVEFORMAT.hpp
    reading/headers/formats/WAVEFORMAT.hpp
    reading/headers/formats/WAVEFORMATEX.hpp
    reading/headers/formats/WAVEFORMATEXTENSIBLE.hpp

    reading/headers/optional_chunks/OptionalChunks.hpp
    reading/headers/optional_chunks/fact.hpp
    reading/headers/optional_chunks/PEAK.hpp

    reading/headers/rejection/Rejection.hpp

    reading/decoding/SampleDecoding.hpp

    reading/file_open/FileOpenFailure.hpp

    reading/invalid_argument/InvalidArgument.hpp

    reading/trailing_bytes/TrailingBytesIgnored.hpp

    utilities/UtilitiesTests.hpp
    utilities/Byteswap.hpp
    utilities/ReadingBytesVector.hpp

    writing/WritingTests.hpp
    writing/HeaderWriting.hpp
    writing/SampleEncoding.hpp
)

source_group (TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${wav_plus_plus_test_headers})

target_sources (tests PRIVATE ${wav_plus_plus_test_headers})

target_link_libraries (tests PRIVATE wav_plus_plus)

target_compile_options (tests PRIVATE ${wav_plus_plus_warning_flags})

set_target_properties (tests PROPERTIES CXX_EXTENSIONS OFF)

add_test (NAME tests COMMAND tests)
