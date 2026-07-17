// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_Rejection_hpp
#define wavplusplus_tests_Rejection_hpp

void TestForReadError (std::vector<uint8_t> const &, TestLog &, std::string const &, wav::ReadErrorCode);
void UpdateRIFFChunkSize (std::vector<uint8_t> &);

#include "RIFF.hpp"
#include "fmt.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <format>
#include <string>
#include <vector>

void RunGenericChunkTests (TestLog &);
void RunDataChunkTests    (TestLog &);
void RunFactChunkTests    (TestLog &);
void RunPeakChunkTests    (TestLog &);

inline void TestRejection (TestLog & log)
{
    RunRiffChunkTests    (log);
    RunGenericChunkTests (log);
    RunFormatChunkTests  (log);
    RunDataChunkTests    (log);
    RunFactChunkTests    (log);
    RunPeakChunkTests    (log);
}

// GENERIC CHUNK TESTS //////////////////////////////////////////////////////////////

inline void DeclaredChunkEndExceedsRiffEnd (TestLog & log)
{
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8);
    PushFourCC (bytes, "chnk");
    PushUint32LE (bytes, 1u); // no body bytes; declared end (incl. pad) past riffEnd
    UpdateRIFFChunkSize (bytes);
    TestForReadError (bytes, log,
                      "Header Rejection - declared sub-chunk end exceeds RIFF end",
                      wav::ReadErrorCode::DeclaredChunkEndExceedsRiffEnd);
}
inline void IncompleteChunkHeader (TestLog & log)
{
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8);
    // Add four bytes which is not enough for a chunk ID and size field
    bytes.push_back (0);
    bytes.push_back (0);
    bytes.push_back (0);
    bytes.push_back (0);
    UpdateRIFFChunkSize (bytes);
    TestForReadError (bytes, log,
                      "Header Rejection - RIFF chunk, incomplete sub-chunk header",
                      wav::ReadErrorCode::IncompleteChunkHeader);
}

inline void RunGenericChunkTests (TestLog & log)
{
    DeclaredChunkEndExceedsRiffEnd (log);
    IncompleteChunkHeader (log);
}

// DATA CHUNK TESTS ///////////////////////////////////////////////////////////////

inline void DataChunkSizeNotDivisibleByBlockAlign (TestLog & log)
{
    // block align: 5 x 24 / 8 = 15 bytes
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 24, 24, 5, 44100, 1);
    OverwriteUint32LE (bytes, chunkOffset ("data", bytes) + 4, 16);
    TestForReadError (bytes, log, "Header Rejection - data chunk, size not divisible by block align", wav::ReadErrorCode::DataChunkSizeNotDivisibleByBlockAlign);
}
inline void MultipleDataChunks (TestLog & log)
{
    constexpr uint16_t bitDepth {8};
    constexpr uint16_t containerSize {bitDepth};
    constexpr uint16_t channels {1};
    constexpr uint32_t frames {8};

    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, bitDepth, containerSize, channels, 44100, frames);

    // 2nd data chunk
    const uint32_t dataChunkSize {frames * channels * (containerSize / 8u)};
    PushSilentDataChunk (bytes, dataChunkSize);
    UpdateRIFFChunkSize (bytes);

    TestForReadError (bytes, log, "Header Rejection - data chunk, multiple data chunks", wav::ReadErrorCode::MultipleDataChunks);
}
inline void MissingDataChunk (TestLog & log)
{
    constexpr auto formatChunkSize {wav::FormatChunkSize::PCMWAVEFORMAT};
    constexpr auto formatCode {wav::FormatCode::LinearPCM};
    constexpr uint16_t channels {1};
    constexpr uint32_t sampleRate {44100};
    constexpr uint16_t blockAlign {1};
    constexpr uint32_t byteRate {sampleRate * blockAlign};

    std::vector<uint8_t> bytes;

    // RIFF chunk header
    PushFourCC    (bytes, "RIFF");
    PushUint32LE  (bytes, 4u + (4u + 4u + formatChunkSize)); // "WAVE" + ("fmt " + fmtChunkSize + fmt chunk body)
    PushFourCC    (bytes, "WAVE");

    // fmt chunk (PCMWAVEFORMAT)
    PushFmtHeader          (bytes, formatChunkSize);
    Push_WAVEFORMAT_Fmt    (bytes, formatCode, channels, sampleRate, byteRate, blockAlign);
    Push_PCMWAVEFORMAT_Fmt (bytes, 8);

    TestForReadError (bytes, log, "Header Rejection - data chunk, missing data chunk", wav::ReadErrorCode::MissingDataChunk);
}
inline void DataChunkBeforeFmtChunk (TestLog & log)
{
    constexpr uint32_t frames {8};

    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, frames);

    const auto fmtChunkStart {chunkOffset ("fmt ", bytes)};
    std::vector<uint8_t> dataChunk;
    PushSilentDataChunk (dataChunk, frames); // pushing to dataChunk, not bytes
    bytes.insert (bytes.begin() + static_cast<std::ptrdiff_t> (fmtChunkStart), dataChunk.begin(), dataChunk.end());
    UpdateRIFFChunkSize (bytes);

    TestForReadError (bytes, log, "Header Rejection - data chunk, data before fmt", wav::ReadErrorCode::DataChunkBeforeFmtChunk);
}

inline void RunDataChunkTests (TestLog & log)
{
    DataChunkSizeNotDivisibleByBlockAlign (log);
    MultipleDataChunks (log);
    MissingDataChunk (log);
    DataChunkBeforeFmtChunk (log);
}

// FACT CHUNK TESTS ///////////////////////////////////////////////////////////////

inline void FactChunkSizeNotFour (TestLog & log)
{
    constexpr uint32_t frames {8};
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::FloatPCM, 32, 32, 2, 48000, frames);
    const auto factChunkStart {bytes.size()};
    PushFactChunk (bytes, frames);
    // Rewrite fact chunk size to invalid value of 2.
    // (Must be smaller than 4, not bigger, to not trigger DeclaredChunkEndExceedsRiffEnd error.)
    OverwriteUint32LE (bytes, factChunkStart + 4, 2u);
    UpdateRIFFChunkSize (bytes);
    TestForReadError (bytes, log, "Header Rejection - fact chunk, size not 4", wav::ReadErrorCode::FactChunkSizeNotFour);
}
inline void MultipleFactChunks (TestLog & log)
{
    constexpr uint32_t frames {8};
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, frames);
    PushFactChunk (bytes, frames);
    PushFactChunk (bytes, frames);
    UpdateRIFFChunkSize (bytes);
    TestForReadError (bytes, log, "Header Rejection - fact chunk, multiple fact chunks", wav::ReadErrorCode::MultipleFactChunks);
}

inline void RunFactChunkTests (TestLog & log)
{
    FactChunkSizeNotFour (log);
    MultipleFactChunks (log);
}

// PEAK CHUNK TESTS ///////////////////////////////////////////////////////////////

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

inline void RunPeakChunkTests (TestLog & log)
{
    PeakChunkBeforeFmtChunk (log);
    PeakChunkSizeMismatchesChannels (log);
    PeakChunkVersionUnsupported (log);
    MultiplePeakChunks (log);
    PeakChunkSizeTooSmallForVersionField (log);
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
