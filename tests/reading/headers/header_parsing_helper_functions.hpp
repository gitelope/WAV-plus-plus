// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_reading_tests_helper_functions_hpp
#define wavplusplus_reading_tests_helper_functions_hpp

#include "../reading_helper_functions.hpp"
#include "../../../include/wav-plus-plus/ChunkInfo.hpp" // for wav::Peak

inline uint32_t riffChunkSize (uint32_t formatChunkSize, uint32_t dataChunkSize)
{
    return 4
    + (8 + formatChunkSize)
    + (8 + dataChunkSize + (dataChunkSize % 2));
}

inline void PushRiffHeader (std::vector<uint8_t> & v, uint32_t formatChunkSize, uint32_t dataChunkSize)
{
    // RIFF header
    PushFourCC   (v, "RIFF");
    PushUint32LE (v, riffChunkSize (formatChunkSize, dataChunkSize));
    PushFourCC   (v, "WAVE");
}

inline void PushSilentDataChunk (std::vector<uint8_t> & v, uint32_t dataChunkSize)
{
    // header
    PushFourCC   (v, "data");
    PushUint32LE (v, dataChunkSize);

    // silence
    v.resize (v.size() + dataChunkSize, 0);

    // padding
    if (dataChunkSize % 2) v.push_back (0);
}

inline void PushDataChunk (std::vector<uint8_t> & v, std::vector<uint8_t> const & data)
{
    // header
    PushFourCC   (v, "data");
    PushUint32LE (v, static_cast<uint32_t> (data.size()));

    v.insert (v.end(), data.cbegin(), data.cend());
    if (data.size() % 2) v.push_back (0);
}

inline void PushFmtHeader (std::vector<uint8_t> & v, uint32_t formatChunkSize)
{
    PushFourCC   (v, "fmt ");
    PushUint32LE (v, formatChunkSize);
}

inline void PushFactChunk (std::vector<uint8_t> & v, uint32_t sampleLength)
{
    PushFourCC   (v, "fact");
    PushUint32LE (v, 4);
    PushUint32LE (v, sampleLength);
}

inline void PushPeakChunk (std::vector<uint8_t> & v, std::vector<wav::Peak> const & peaks, uint32_t timestamp)
{
    // chunk size: version (4 bytes) + timestamp (4 bytes) + peaks (8 bytes per peak)
    const auto chunkSize = 4u + 4u + static_cast<uint32_t> (peaks.size()) * 8u;

    PushFourCC   (v, "PEAK");
    PushUint32LE (v, chunkSize);
    PushUint32LE (v, 1);  // version
    PushUint32LE (v, timestamp); // seconds since Jan 1, 1970
    for (auto const & p : peaks)
    {
        PushFloat32LE (v, p.value);
        PushUint32LE  (v, p.positionInFrames);
    }
}

inline size_t chunkOffset (const char id[4], std::vector<uint8_t> const & bytes)
{
    for (size_t i = 0; i < bytes.size(); ++i)
    {
        if (bytes[i] == id[0] && bytes[i+1] == id[1] && bytes[i+2] == id[2] && bytes[i+3] == id[3])
            return i;
    }
    throw std::runtime_error (std::format ("Didn't find chunk: {}", id));
}

inline void Push_WAVEFORMAT_Fmt (std::vector<uint8_t> & v, uint16_t formatCode,
                                 uint16_t channels, uint32_t sampleRate,
                                 uint32_t byteRate, uint16_t blockAlign)
{
    PushUint16LE (v, formatCode);
    PushUint16LE (v, channels);
    PushUint32LE (v, sampleRate);
    PushUint32LE (v, byteRate);
    PushUint16LE (v, blockAlign);
}

inline void Push_PCMWAVEFORMAT_Fmt (std::vector<uint8_t> & v, uint16_t bitDepth)
{
    PushUint16LE (v, bitDepth);
}

inline void Push_WAVEFORMATEX_Fmt (std::vector<uint8_t> & v, uint16_t cbSize)
{
    PushUint16LE (v, cbSize);
}

inline void Push_WAVEFORMATEXTENSIBLE_Fmt (std::vector<uint8_t> & v, uint16_t validBitsPerSample, uint32_t channelMask, wav::GUID subformat)
{
    PushUint16LE (v, validBitsPerSample);
    PushUint32LE (v, channelMask);
    PushGUID     (v, subformat);
}

#endif
