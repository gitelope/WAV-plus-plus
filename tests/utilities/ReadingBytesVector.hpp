// WAV++
// Copyright (c) 2025 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_ReadingBytesVector_hpp
#define wavplusplus_tests_ReadingBytesVector_hpp

#include "../../include/wav-plus-plus/private/ReadingBytes.hpp"

#include <cstddef>
#include <format>
#include <string>
#include <vector>

inline void TestForReadError (TestLog &, std::string const &, wav::ReadErrorCode, auto const &);

inline void TestReadingBytesVector (TestLog & log)
{
    constexpr wav::ReadErrorCode expectedErrorCode {wav::ReadErrorCode::ReadPastEndOfVector};

    {
        std::vector<std::byte> bytes;
        size_t pos {0};
        TestForReadError (log, "Utilities Rejection - read past end of byte vector, uint8",
                          expectedErrorCode,
                          [&] { (void) wav::uint8 (bytes, pos); });
    }
    {
        std::vector<std::byte> bytes (1, std::byte {0});
        size_t pos {0};
        TestForReadError (log,
                          "Utilities Rejection - read past end of byte vector, int16_le",
                          expectedErrorCode,
                          [&] { (void) wav::int16_le (bytes, pos); });
    }
    {
        std::vector<std::byte> bytes (2, std::byte {0});
        size_t pos {0};
        TestForReadError (log, "Utilities Rejection - read past end of byte vector, int24_le",
                          expectedErrorCode,
                          [&] { (void) wav::int24_le (bytes, pos); });
    }
    {
        std::vector<std::byte> bytes (3, std::byte {0});
        size_t pos {0};
        TestForReadError (log, "Utilities Rejection - read past end of byte vector, int32_le",
                          expectedErrorCode,
                          [&] { (void) wav::int32_le (bytes, pos); });
    }
    {
        std::vector<std::byte> bytes (3, std::byte {0});
        size_t pos {0};
        TestForReadError (log, "Utilities Rejection - read past end of byte vector, float32_le",
                          expectedErrorCode,
                          [&] { (void) wav::float32_le (bytes, pos); });
    }
    {
        std::vector<std::byte> bytes (7, std::byte {0});
        size_t pos {0};
        TestForReadError (log,
                          "Utilities Rejection - read past end of byte vector, float64_le",
                          expectedErrorCode,
                          [&] { (void) wav::float64_le (bytes, pos); });
    }
}

inline void TestForReadError (TestLog & log,
                              std::string const & label,
                              wav::ReadErrorCode expectedCode,
                              auto const & readFunctionToTest)
{
    bool passes {false};
    std::string errorMessage;
    try
    {
        readFunctionToTest();
    }
    catch (wav::ReadError const & e)
    {
        if (e.code() == expectedCode) passes = true;
        else errorMessage = e.what();
    }

    if (passes)
    {
        log.Record (passes, label);
    }
    else
    {
        log.Record (passes, std::format ("{} - {}", label, errorMessage));
    }
}

#endif

