// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_optional_chunks_PEAK_hpp
#define wavplusplus_tests_optional_chunks_PEAK_hpp

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <format>
#include <string>
#include <vector>

inline void TestPeak (TestLog & log)
{
    const auto filename {"test_peak_header_parsing.wav"};
    constexpr auto formatCode {wav::FormatCode::FloatPCM};
    constexpr uint16_t channels            {2};
    constexpr uint16_t bitDepth            {32};
    constexpr uint32_t frames              {8};

    constexpr float peakL {0.5f};
    constexpr float peakR {-0.3f};
    constexpr uint32_t peakPositionL {2};
    constexpr uint32_t peakPositionR {5};

    // Construct RIFF, fmt, and data chunks:
    auto bytes = buildPCMWAVEFORMAT (formatCode, bitDepth, bitDepth, channels, 48000, frames);

    // Add PEAK chunk:
    const std::vector<wav::Peak> peaks
    {
        {peakL, peakPositionL},
        {peakR, peakPositionR}
    };
    const uint32_t timestamp {17}; // seconds elapsed since Jan 1, 1970
    PushPeakChunk (bytes, peaks, timestamp);
    // Update RIFF chunk size to reflect PEAK chunk size:
    OverwriteUint32LE (bytes, 4, static_cast<uint32_t> (bytes.size() - 8));

    WriteBytesToFile (filename, bytes);

    constexpr uint32_t expectedSize {8 + 8 * channels}; // 4 bytes for version, 4 bytes for timestamp, 8 bytes per peak
    constexpr uint32_t expectedVersion {1}; // version 1 is the only supported version
    constexpr uint32_t expectedTimestamp {17};

    std::string failures;
    try
    {
        wav::ChunkInfo info {filename};

        Compare (info.peak.present      == true,              "PEAK chunk not detected",   failures);
        Compare (info.peak.size         == expectedSize,      "unexpected peak.size",      failures);
        Compare (info.peak.version      == expectedVersion,   "unexpected peak.version",   failures);
        Compare (info.peak.timestamp    == expectedTimestamp, "unexpected peak.timestamp", failures);
        Compare (info.peak.peaks.size() == peaks.size(),      "unexpected peak count",     failures);
        for (size_t i = 0; i < peaks.size(); ++i)
        {
            Compare (info.peak.peaks[i].value == peaks[i].value,
                     std::format ("unexpected peak[{}] value", i), failures);
            Compare (info.peak.peaks[i].positionInFrames == peaks[i].positionInFrames,
                     std::format ("unexpected peak[{}] position", i), failures);
        }
    }
    catch (wav::ReadError const & e)
    {
        failures = std::string {"wav::ReadError - "} + e.what();
    }
    const auto suffix {failures.empty() ? "" : ", failures: " + failures};
    log.Record (failures.empty(), std::format ("Read Header - optional chunks - PEAK parsed correctly{}", suffix));

    std::remove (filename);
}

#endif
