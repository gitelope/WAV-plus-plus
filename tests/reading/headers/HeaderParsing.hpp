
#ifndef wavplusplus_tests_ReadingHeaders_Tests_hpp
#define wavplusplus_tests_ReadingHeaders_Tests_hpp

#include "header_parsing_helper_functions.hpp"
#include "../../../include/wav-plus-plus/Reader.hpp"

#include <cassert>
#include <format>

#include "WAVEFORMAT.hpp"
#include "PCMWAVEFORMAT.hpp"
#include "WAVEFORMATEX.hpp"
#include "WAVEFORMATEXTENSIBLE.hpp"

void TestHeaderParsing (TestLog & log)
{
    TestWAVEFORMAT           (log);
    TestPCMWAVEFORMAT        (log);
    TestWAVEFORMATEX         (log);
    TestWAVEFORMATEXTENSIBLE (log);
}

#endif

