// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_HeaderParsing_hpp
#define wavplusplus_tests_HeaderParsing_hpp

#include "header_parsing_helper_functions.hpp"
#include "../../../include/wav-plus-plus/Reader.hpp"

#include <cassert>
#include <format>

#include "formats/Formats.hpp"
#include "optional_chunks/OptionalChunks.hpp"
#include "rejection/Rejection.hpp"

void TestHeaderParsing (TestLog & log)
{
    TestFormats        (log);
    TestOptionalChunks (log);
    TestRejection      (log);
}

#endif

