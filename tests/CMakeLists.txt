
add_executable (tests tests.cpp)

set (wav_plus_plus_test_headers
    TestLog.hpp
    helper_functions.hpp

    reading/ReadingTests.hpp
    reading/decoding/SampleDecoding.hpp
    reading/headers/HeaderParsing.hpp
    reading/headers/PCMWAVEFORMAT.hpp
    reading/headers/WAVEFORMAT.hpp
    reading/headers/WAVEFORMATEX.hpp
    reading/headers/WAVEFORMATEXTENSIBLE.hpp
    reading/headers/header_parsing_helper_functions.hpp
    reading/reading_helper_functions.hpp

    utilities/Byteswap.hpp
    utilities/UtilitiesTests.hpp

    writing/HeaderWriting.hpp
    writing/SampleEncoding.hpp
    writing/WritingTests.hpp
)

source_group (TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${wav_plus_plus_test_headers})

target_sources (tests PRIVATE ${wav_plus_plus_test_headers})

target_link_libraries (tests PRIVATE wav_plus_plus)

target_compile_options (tests PRIVATE ${wav_plus_plus_warning_flags})

set_target_properties (tests PROPERTIES CXX_EXTENSIONS OFF)

add_test (NAME tests COMMAND tests)
