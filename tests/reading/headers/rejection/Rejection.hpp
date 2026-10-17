// WAV++
// Copyright (c) 2025 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_Rejection_hpp
#define wavplusplus_tests_Rejection_hpp

#include <cstdint>
#include <string>
#include <vector>

void TestForReadError (std::vector<uint8_t> const &, TestLog &, std::string const &, wav::ReadErrorCode);
void UpdateRIFFChunkSize (std::vector<uint8_t> &);

#include "RIFF.hpp"
#include "fmt.hpp"
#include "data.hpp"
#include "fact.hpp"
#include "PEAK.hpp"
#include "generic.hpp"

#include <cstdio>
#include <format>

inline void TestRejection (TestLog & log)
{
    RunRiffChunkTests    (log);
    RunGenericChunkTests (log);
    RunFormatChunkTests  (log);
    RunDataChunkTests    (log);
    RunFactChunkTests    (log);
    RunPeakChunkTests    (log);
}

// HELPER FUNCTIONS ///////////////////////////////////////////////////////////////

inline void TestForReadError (std::vector<uint8_t> const & bytes,
                              TestLog & log,
                              std::string const & label,
                              wav::ReadErrorCode expectedCode)
{
    const auto filename {"test_rejection.wav"};
    WriteBytesToFile (filename, bytes);

    bool passes {false};
    std::string errorMessage;
    try
    {
        wav::Reader reader {filename};
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

    std::remove (filename);
}

inline void UpdateRIFFChunkSize (std::vector<uint8_t> & bytes)
{
    OverwriteUint32LE (bytes, 4, static_cast<uint32_t> (bytes.size() - 8));
}

#endif
