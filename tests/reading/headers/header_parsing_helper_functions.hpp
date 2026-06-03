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

inline void push_riff_header (std::vector<uint8_t> & v, uint32_t formatChunkSize, uint32_t dataChunkSize)
{
    // RIFF header
    push_fourCC    (v, "RIFF");
    push_uint32_le (v, riffChunkSize (formatChunkSize, dataChunkSize));
    push_fourCC    (v, "WAVE");
}

inline void push_silent_data_chunk (std::vector<uint8_t> & v, uint32_t dataChunkSize)
{
    // header
    push_fourCC    (v, "data");
    push_uint32_le (v, dataChunkSize);

    // silence
    v.resize (v.size() + dataChunkSize, 0);

    // padding
    if (dataChunkSize % 2) v.push_back (0);
}

inline void push_data_chunk (std::vector<uint8_t> & v, std::vector<uint8_t> const & data)
{
    // header
    push_fourCC    (v, "data");
    push_uint32_le (v, static_cast<uint32_t> (data.size()));

    v.insert (v.end(), data.cbegin(), data.cend());
    if (data.size() % 2) v.push_back (0);
}

inline void push_fmt_header (std::vector<uint8_t> & v, uint32_t formatChunkSize)
{
    push_fourCC    (v, "fmt ");
    push_uint32_le (v, formatChunkSize);
}

inline void push_fact_chunk (std::vector<uint8_t> & v, uint32_t sampleLength)
{
    push_fourCC    (v, "fact");
    push_uint32_le (v, 4);
    push_uint32_le (v, sampleLength);
}

inline void push_peak_chunk (std::vector<uint8_t> & v, std::vector<wav::Peak> const & peaks, uint32_t timestamp)
{
    // chunk size: version (4 bytes) + timestamp (4 bytes) + peaks (8 bytes per peak)
    const auto chunkSize = 4u + 4u + static_cast<uint32_t> (peaks.size()) * 8u;

    push_fourCC    (v, "PEAK");
    push_uint32_le (v, chunkSize);
    push_uint32_le (v, 1);  // version
    push_uint32_le (v, timestamp); // seconds since Jan 1, 1970
    for (auto const & p : peaks)
    {
        push_float32_le (v, p.value);
        push_uint32_le  (v, p.positionInFrames);
    }
}

inline void push_WAVEFORMAT_fmt (std::vector<uint8_t> & v, uint16_t formatCode,
                                 uint16_t channels, uint32_t sampleRate,
                                 uint32_t byteRate, uint16_t blockAlign)
{
    push_uint16_le (v, formatCode);
    push_uint16_le (v, channels);
    push_uint32_le (v, sampleRate);
    push_uint32_le (v, byteRate);
    push_uint16_le (v, blockAlign);
}

inline void push_PCMWAVEFORMAT_fmt (std::vector<uint8_t> & v, uint16_t bitDepth)
{
    push_uint16_le (v, bitDepth);
}

inline void push_WAVEFORMATEX_fmt (std::vector<uint8_t> & v, uint16_t cbSize)
{
    push_uint16_le (v, cbSize);
}

inline void push_WAVEFORMATEXTENSIBLE_fmt (std::vector<uint8_t> & v, uint16_t validBitsPerSample, uint32_t channelMask, wav::GUID subformat)
{
    push_uint16_le (v, validBitsPerSample);
    push_uint32_le (v, channelMask);
    push_GUID      (v, subformat);
}

#endif
