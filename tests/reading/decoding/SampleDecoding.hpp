// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_SampleDecoding_hpp
#define wavplusplus_tests_SampleDecoding_hpp

#include "FullContainer.hpp"
#include "PaddedLPCM.hpp"

inline void TestSampleDecoding (TestLog & log)
{
    TestFullContainer (log); // the bulk of the tests are here
    TestPaddedLPCM (log);
}

#endif
