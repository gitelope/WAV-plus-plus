
#ifndef wavplusplus_tests_WritingTests_hpp
#define wavplusplus_tests_WritingTests_hpp

#include "SampleEncoding.hpp"
#include "HeaderWriting.hpp"

void TestWriting (TestLog & log)
{
    TestHeaders (log);
    TestSpeakerPosition (log);
    TestSpeakerPositionValidation (log);

    TestSampleEncoding (log);
}

#endif
