// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_data_hpp
#define wavplusplus_tests_data_hpp

#include <cstddef>
#include <cstdint>
#include <vector>

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

#endif
