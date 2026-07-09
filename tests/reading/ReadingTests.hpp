// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_ReadingTests_hpp
#define wavplusplus_tests_ReadingTests_hpp

#include "../../include/wav-plus-plus/Reader.hpp"
#include "reading_helper_functions.hpp"

#include "headers/HeaderParsing.hpp"
#include "decoding/SampleDecoding.hpp"
#include "invalid_argument/InvalidArgument.hpp"
#include "file_open/FileOpenFailure.hpp"
#include "trailing_bytes/TrailingBytesIgnored.hpp"

void TestReading (TestLog & log)
{
    TestHeaderParsing (log);
    TestSampleDecoding (log);
    TestInvalidArgument (log);
    TestFileOpenFailure (log);
    TestTrailingBytesIgnored (log);
}

#endif
