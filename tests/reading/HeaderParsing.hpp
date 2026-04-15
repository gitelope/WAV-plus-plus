
#ifndef wavplusplus_tests_ReadingHeaders_Tests_hpp
#define wavplusplus_tests_ReadingHeaders_Tests_hpp

#include "../../include/wav-plus-plus/Reader.hpp"
#include "reading_tests_helper_functions.hpp"
#include "TestWAVEFORMAT.hpp"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

void TestWAVEFORMATEX         (TestLog & log);
void TestWAVEFORMATEXTENSIBLE (TestLog & log);

void TestHeaderParsing (TestLog & log)
{
    TestWAVEFORMAT    (log);
    //TestPCMWAVEFORMAT (log);
}

#endif

