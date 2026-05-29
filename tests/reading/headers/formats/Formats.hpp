// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_Formats_hpp
#define wavplusplus_tests_Formats_hpp

#include "WAVEFORMAT.hpp"
#include "PCMWAVEFORMAT.hpp"
#include "WAVEFORMATEX.hpp"
#include "WAVEFORMATEXTENSIBLE.hpp"

void TestFormats (TestLog & log)
{
    TestWAVEFORMAT           (log);
    TestPCMWAVEFORMAT        (log);
    TestWAVEFORMATEX         (log);
    TestWAVEFORMATEXTENSIBLE (log);
}

#endif

