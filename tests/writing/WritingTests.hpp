// WAV++ (c) 2025 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_WritingTests_hpp
#define wavplusplus_tests_WritingTests_hpp

#include "../../include/wav-plus-plus/Writer.hpp"

#include "SampleEncoding.hpp"
#include "HeaderWriting.hpp"
#include "invalid_argument/InvalidArgument.hpp"
#include "unsupported_format/UnsupportedFormat.hpp"
#include "file_open/FileOpenFailure.hpp"

void TestWriting (TestLog & log)
{
    TestHeaders (log);
    TestSampleEncoding (log);
    TestInvalidArgumentForWriting (log);
    TestUnsupportedFormat (log);
    TestFileOpenForWritingFailure (log);
}

#endif
