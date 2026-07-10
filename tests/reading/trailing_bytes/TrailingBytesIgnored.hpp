// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_TrailingBytesIgnored_hpp
#define wavplusplus_tests_TrailingBytesIgnored_hpp

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <format>
#include <string>

inline void TestTrailingBytesIgnored (TestLog & log)
{
    const auto filename {"test_trailing_bytes.wav"};

    constexpr uint32_t frames {8};
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, frames);

    const auto fmtStart {chunkOffset ("fmt ", bytes)};
    const auto dataStart {chunkOffset ("data", bytes)};

    // Copy fmt chunk and paste at the end.
    // It should be ignored by the parser since it exists past the RIFF chunk end.
    bytes.insert (bytes.end(),
                  bytes.begin() + static_cast<std::ptrdiff_t> (fmtStart),
                  bytes.begin() + static_cast<std::ptrdiff_t> (dataStart));

    WriteBytesToFile (filename, bytes);

    std::string failures;
    try
    {
        wav::ChunkInfo info {filename};
        size_t fmtCount {0};
        for (auto const & entry : info.chunksEncountered)
        {
            if (entry.starts_with ("fmt ")) ++fmtCount;
        }
        Compare (fmtCount == 1, "expected exactly one fmt chunk in chunksEncountered", failures);
    }
    catch (wav::ReadError const & e)
    {
        failures = std::string {"unexpected wav::ReadError - "} + e.what();
    }

    const auto suffix {failures.empty() ? "" : ", failures: " + failures};
    log.Record (failures.empty(),
                std::format ("Trailing bytes ignored - second fmt past RIFF size not parsed{}", suffix));

    std::remove (filename);
}


#endif
