
#ifndef wavplusplus_tests_ReadingTests_hpp
#define wavplusplus_tests_ReadingTests_hpp

#include "HeaderParsing.hpp"
#include "SampleDecoding.hpp"

void TestReading (TestLog & log)
{
    TestHeaderParsing (log);
    TestSampleDecoding (log);
}

#endif
