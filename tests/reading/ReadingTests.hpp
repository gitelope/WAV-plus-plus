// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_ReadingTests_hpp
#define wavplusplus_tests_ReadingTests_hpp

#include "headers/HeaderParsing.hpp"
#include "decoding/SampleDecoding.hpp"
#include "invalid_argument/InvalidArgument.hpp"

void TestReading (TestLog & log)
{
    TestHeaderParsing (log);
    TestSampleDecoding (log);
    TestInvalidArgument (log);
}

#endif
