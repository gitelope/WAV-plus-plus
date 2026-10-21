// WAV++
// Copyright (c) 2025 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_WritingTests_hpp
#define wavplusplus_tests_WritingTests_hpp

#include "../../include/wav-plus-plus/Writer.hpp"

#include "SampleEncoding.hpp"
#include "HeaderWriting.hpp"
#include "invalid_argument/InvalidArgument.hpp"
#include "file_open/FileOpenFailure.hpp"

void TestWriting (TestLog & log)
{
    TestHeaderWriting (log);
    TestSampleEncoding (log);
    TestInvalidArgumentForWriting (log);
    TestFileOpenForWritingFailure (log);
}

#endif
