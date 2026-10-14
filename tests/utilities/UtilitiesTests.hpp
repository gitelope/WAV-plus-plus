// WAV++ (c) 2025 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_UtilitiesTests_hpp
#define wavplusplus_tests_UtilitiesTests_hpp

#include "Byteswap.hpp"
#include "ReadingBytesVector.hpp"

void TestUtilities (TestLog & log)
{
    TestByteswap (log);
    TestReadingBytesVector (log);
}

#endif
