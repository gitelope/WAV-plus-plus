// WAV++
// Copyright (c) 2025 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_rejection_PEAK_hpp
#define wavplusplus_tests_rejection_PEAK_hpp

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <format>
#include <string>
#include <vector>

inline void MultiplePeakChunks (TestLog & log)
{
    constexpr uint32_t frames {8};
    constexpr uint16_t channels {2};

    const std::vector<wav::Peak> peaks
    {
        {0,0}, // peak of 0 at frame 0 for channel 0
        {0,0}  // peak of 0 at frame 0 for channel 1
    };
    assert (peaks.size() == channels);

    const uint32_t timestamp {0};

    // First PEAK is well-formed version 1
    {
        auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, channels, 44100, frames);
        PushPeakChunk (bytes, peaks, timestamp);
        PushPeakChunk (bytes, peaks, timestamp);
        UpdateRIFFChunkSize (bytes);
        TestForReadError (bytes, log, "Header Rejection - PEAK chunk, multiple PEAK chunks", wav::ReadErrorCode::MultiplePeakChunks);
    }

    // First PEAK is version 2 (skipped); ReadError still thrown when 2nd PEAK chunk encountered
    {
        auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, channels, 44100, frames);
        const auto peakChunkStart {bytes.size()};
        PushPeakChunk (bytes, peaks, timestamp);
        OverwriteUint32LE (bytes, peakChunkStart + 8, 2); // Set version to 2.
        PushPeakChunk (bytes, peaks, timestamp);
        UpdateRIFFChunkSize (bytes);
        TestForReadError (bytes, log, "Header Rejection - PEAK chunk, multiple PEAK chunks (1st skipped due to unsupported version; 2nd throws ReadError)", wav::ReadErrorCode::MultiplePeakChunks);
    }
}
inline void PeakChunkBeforeFmtChunk (TestLog & log)
{
    constexpr uint16_t channels {2};

    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::FloatPCM, 32, 32, channels, 48000, 8);

    std::vector<wav::Peak> peaks;
    for (auto n = channels; n > 0; --n) peaks.push_back ({0.f, 0u});

    // insert PEAK chunk before fmt chunk:
    std::vector<uint8_t> peakChunk;
    PushPeakChunk (peakChunk, peaks, 0);
    // RIFF id, chunk size, and "WAVE" occupy the first 12 bytes:
    bytes.insert (bytes.begin() + 12, peakChunk.begin(), peakChunk.end());
    UpdateRIFFChunkSize (bytes);

    TestForReadError (bytes, log, "Header Rejection - PEAK chunk, PEAK comes before fmt", wav::ReadErrorCode::PeakChunkBeforeFmtChunk);
}
inline void PeakChunkSizeMismatchesChannels (TestLog & log)
{
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::FloatPCM, 32, 32, 2, 48000, 8);
    // WAV has 2 channels but PEAK chunk size is not 8 + 8 * 2 = 24; it is 16.
    const std::vector<wav::Peak> peaks {{0.f, 0u}};
    PushPeakChunk (bytes, peaks, 0);
    UpdateRIFFChunkSize (bytes);

    TestForReadError (bytes, log, "Header Rejection - PEAK chunk, size mismatches channels", wav::ReadErrorCode::PeakChunkSizeMismatchesChannels);
}
inline void PeakChunkSizeTooSmallForVersionField (TestLog & log)
{
    constexpr uint16_t channels {2};

    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::FloatPCM, 32, 32, channels, 48000, 8);

    const auto peakChunkStart {bytes.size()};
    std::vector<wav::Peak> peaks;
    for (auto n = channels; n > 0; --n) peaks.push_back ({0.f, 0u});
    PushPeakChunk (bytes, peaks, 0);
    OverwriteUint32LE (bytes, peakChunkStart + 4, 2); // version field needs 4 bytes in chunk body
    UpdateRIFFChunkSize (bytes);

    TestForReadError (bytes, log, "Header Rejection - PEAK chunk, size too small for version field", wav::ReadErrorCode::PeakChunkSizeTooSmallForVersionField);
}
inline void PeakChunkVersionUnsupported (TestLog & log)
{
    // Since PEAK is optional, we can skip any chunk that isn't version 1 instead of throwing.

    const auto filename {"test_peak_version_unsupported.wav"};
    constexpr uint16_t channels {2};

    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::FloatPCM, 32, 32, channels, 48000, 8);

    const auto peakChunkStart {bytes.size()};
    std::vector<wav::Peak> peaks;
    for (auto n = channels; n > 0; --n) peaks.push_back ({0.f, 0u});
    PushPeakChunk (bytes, peaks, 0);
    // Set version to 2.
    OverwriteUint32LE (bytes, peakChunkStart + 8, 2);
    UpdateRIFFChunkSize (bytes);

    WriteBytesToFile (filename, bytes);

    std::string failures;
    try
    {
        wav::ChunkInfo info {filename};

        Compare (info.peak.present == false, "peak.present should be false when version != 1", failures);

        // Check that "PEAK <chunkStart>" appears in chunksSkipped.
        const auto expected {std::format ("PEAK {}", peakChunkStart)};
        bool listedAsSkipped {false};
        for (auto const & str : info.chunksSkipped)
        {
            if (str == expected)
            {
                listedAsSkipped = true;
                break;
            }
        }
        Compare (listedAsSkipped, "PEAK should appear in chunksSkipped", failures);
    }
    catch (wav::ReadError const & e)
    {
        failures = std::string {"unexpected wav::ReadError - "} + e.what();
    }
    const auto suffix {failures.empty() ? "" : ", failures: " + failures};

    log.Record (failures.empty(), std::format ("Skip Chunk - PEAK, version not 1{}", suffix));

    std::remove (filename);
}

inline void RunPeakChunkTests (TestLog & log)
{
    MultiplePeakChunks (log);
    PeakChunkBeforeFmtChunk (log);
    PeakChunkSizeMismatchesChannels (log);
    PeakChunkSizeTooSmallForVersionField (log);
    PeakChunkVersionUnsupported (log);
}

#endif
