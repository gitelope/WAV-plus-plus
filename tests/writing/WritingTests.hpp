// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_WritingTests_hpp
#define wavplusplus_tests_WritingTests_hpp

#include "SampleEncoding.hpp"
#include "HeaderWriting.hpp"

void TestWriting (TestLog & log)
{
    TestHeaders (log);
    TestSampleEncoding (log);
}

#endif
