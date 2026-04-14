
#ifndef wavplusplus_tests_WritingTests_hpp
#define wavplusplus_tests_WritingTests_hpp

#include "SampleEncoding_Tests.hpp"
#include "Header_Tests.hpp"

void TestWriting (TestLog & log)
{
    TestSampleEncoding (log);
    TestHeaders (log);
    TestSpeakerPositionHeader (log);
    TestSpeakerPositionValidation (log);
}

#endif
