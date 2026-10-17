// WAV++
// Copyright (c) 2025 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_reading_helper_functions_hpp
#define wavplusplus_tests_reading_helper_functions_hpp

#include <bit>
#include <cassert>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <format>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

// Write byte vector to a file.
inline void WriteBytesToFile (std::string const & filepath, std::vector<uint8_t> const & bytes)
{
    auto out = wav::fileForWriting (filepath);

    if (std::fwrite (bytes.data(), 1, bytes.size(), out.get()) != bytes.size())
    {
        throw std::runtime_error ("Failed to write file: " + filepath);
    }
}

// Append a little-endian uint16 to byte vector.
inline void PushUint16LE (std::vector<uint8_t> & v, uint16_t value)
{
    v.push_back (static_cast<uint8_t> (value     ));
    v.push_back (static_cast<uint8_t> (value >> 8));
}

inline void OverwriteUint16LE (std::vector<uint8_t> & v, size_t index, uint16_t value)
{
    v[index]     = static_cast<uint8_t> (value     );
    v[index + 1] = static_cast<uint8_t> (value >> 8);
}

// Append a little-endian uint32 to byte vector.
inline void PushUint32LE (std::vector<uint8_t> & v, uint32_t value)
{
    v.push_back (static_cast<uint8_t> (value      ));
    v.push_back (static_cast<uint8_t> (value >> 8 ));
    v.push_back (static_cast<uint8_t> (value >> 16));
    v.push_back (static_cast<uint8_t> (value >> 24));
}

inline void OverwriteUint32LE (std::vector<uint8_t> & v, size_t index, uint32_t value)
{
    v[index]     = static_cast<uint8_t> (value      );
    v[index + 1] = static_cast<uint8_t> (value >>  8);
    v[index + 2] = static_cast<uint8_t> (value >> 16);
    v[index + 3] = static_cast<uint8_t> (value >> 24);
}

// Append a 32-bit float to a byte vector.
inline void PushFloat32LE (std::vector<uint8_t> & v, float value)
{
    static_assert (std::numeric_limits<float>::is_iec559);
    static_assert (sizeof (float) == 4);
    static_assert (CHAR_BIT == 8);
    PushUint32LE (v, std::bit_cast<uint32_t> (value));
}

// Append a 4-character string to a byte vector.
inline void PushFourCC (std::vector<uint8_t> & v, const char * fourCC)
{
    v.push_back (static_cast<uint8_t> (fourCC[0]));
    v.push_back (static_cast<uint8_t> (fourCC[1]));
    v.push_back (static_cast<uint8_t> (fourCC[2]));
    v.push_back (static_cast<uint8_t> (fourCC[3]));
}

inline void PushGUID (std::vector<uint8_t> & v, wav::GUID const & guid)
{
    // GUID layout:
    // data1 (uint32)
    // data2 (uint16)
    // data3 (uint16)
    // data4 (8 byte array)
    PushUint32LE (v, guid.data1);
    PushUint16LE (v, guid.data2);
    PushUint16LE (v, guid.data3);
    for (uint8_t byte : guid.data4)
    {
        v.push_back (byte);
    }
}

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

// Build a WAVEFORMAT WAV file (14-byte fmt chunk)
// with a silent data chunk.
//
// Layout:
//   RIFF header   (12 bytes)
//   fmt  chunk    (8 + 14 = 22 bytes)
//   data chunk    (8 + dataSize bytes)
//   padding byte  (if dataSize is odd)
//
inline std::vector<uint8_t> buildWAVEFORMAT (uint16_t formatCode, uint16_t bitDepth, uint16_t channels,
                                             uint32_t sampleRate, uint32_t frames)
{
    constexpr uint32_t formatChunkSize {wav::FormatChunkSize::WAVEFORMAT};

    assert (bitDepth % 8 == 0);
    const auto blockAlign {static_cast<uint16_t> (channels * bitDepth / 8)};
    const auto byteRate {sampleRate * blockAlign};
    const auto dataChunkSize {frames * blockAlign};

    std::vector<uint8_t> wav;

    // RIFF
    PushRiffHeader (wav, formatChunkSize, dataChunkSize);

    // fmt
    PushFmtHeader        (wav, formatChunkSize);
    Push_WAVEFORMAT_Fmt    (wav, formatCode, channels, sampleRate, byteRate, blockAlign); // 14 bytes

    // data
    PushSilentDataChunk (wav, dataChunkSize);

    return wav;
}

// Build a PCMWAVEFORMAT WAV file (16-byte fmt chunk)
// with a silent data chunk.
//
// Layout:
//   RIFF header   (12 bytes)
//   fmt  chunk    (8 + 16 = 24 bytes)
//   data chunk    (8 + dataSize bytes)
//   padding byte  (if dataSize is odd)
//
inline std::vector<uint8_t> buildPCMWAVEFORMAT (uint16_t formatCode, uint16_t validBitDepth,
                                                uint16_t containerSizeInBits, uint16_t channels,
                                                uint32_t sampleRate, uint32_t frames)
{
    constexpr uint32_t formatChunkSize {wav::FormatChunkSize::PCMWAVEFORMAT};

    assert (containerSizeInBits % 8 == 0);
    const auto blockAlign {static_cast<uint16_t> (channels * containerSizeInBits / 8)};
    const auto byteRate {sampleRate * blockAlign};
    const auto dataChunkSize {frames * blockAlign};

    std::vector<uint8_t> wav;

    // RIFF
    PushRiffHeader (wav, formatChunkSize, dataChunkSize);

    // fmt
    PushFmtHeader          (wav, formatChunkSize);
    Push_WAVEFORMAT_Fmt    (wav, formatCode, channels, sampleRate, byteRate, blockAlign); // 14 bytes
    Push_PCMWAVEFORMAT_Fmt (wav, validBitDepth);                                          //  2 bytes
    // --------
    // 16 bytes

    // data
    PushSilentDataChunk (wav, dataChunkSize);

    return wav;
}

// Build a WAVEFORMATEX WAV file (18-byte fmt chunk)
// with a silent data chunk.
//
// Layout:
//   RIFF header   (12 bytes)
//   fmt  chunk    (8 + 18 = 26 bytes)
//   data chunk    (8 + dataSize bytes)
//   padding byte  (if dataSize is odd)
//
inline std::vector<uint8_t> buildWAVEFORMATEX (uint16_t formatCode, uint16_t validBitDepth, uint16_t containerSizeInBits, uint16_t channels, uint32_t sampleRate, uint32_t frames)
{
    constexpr uint32_t formatChunkSize {wav::FormatChunkSize::WAVEFORMATEX};

    assert (containerSizeInBits % 8 == 0);
    const auto blockAlign {static_cast<uint16_t> (channels * containerSizeInBits / 8)};
    const auto byteRate {sampleRate * blockAlign};
    const auto dataChunkSize {frames * blockAlign};

    const uint16_t cbSize {0};

    std::vector<uint8_t> wav;

    // RIFF
    PushRiffHeader (wav, formatChunkSize, dataChunkSize);

    // fmt
    PushFmtHeader          (wav, formatChunkSize);
    Push_WAVEFORMAT_Fmt    (wav, formatCode, channels, sampleRate, byteRate, blockAlign); // 14 bytes
    Push_PCMWAVEFORMAT_Fmt (wav, validBitDepth);                                          //  2 bytes
    Push_WAVEFORMATEX_Fmt  (wav, cbSize);                                                 //  2 bytes
    // --------
    // 18 bytes

    // data
    PushSilentDataChunk (wav, dataChunkSize);

    return wav;
}

// Build a WAVEFORMATEXTENSIBLE WAV file (40-byte fmt chunk)
// with a silent data chunk.
//
// Layout:
//   RIFF header   (12 bytes)
//   fmt  chunk    (8 + 40 = 48 bytes)
//   data chunk    (8 + dataSize bytes)
//   padding byte  (if dataSize is odd)
//
inline std::vector<uint8_t> buildWAVEFORMATEXTENSIBLE (wav::GUID const & subformat,
                                                       uint16_t validBitDepth, uint16_t containerSizeInBits,
                                                       uint16_t channels, uint32_t sampleRate,
                                                       uint32_t frames, uint32_t channelMask)
{
    constexpr uint32_t formatChunkSize {wav::FormatChunkSize::WAVEFORMATEXTENSIBLE};

    assert (containerSizeInBits % 8 == 0);
    const auto blockAlign {static_cast<uint16_t> (channels * containerSizeInBits / 8)};
    const auto byteRate {sampleRate * blockAlign};
    const auto dataChunkSize {frames * blockAlign};

    const uint16_t cbSize {22};

    const auto formatCode {wav::FormatCode::Extensible};


    std::vector<uint8_t> wav;

    // RIFF
    PushRiffHeader (wav, formatChunkSize, dataChunkSize);

    // fmt
    PushFmtHeader                 (wav, formatChunkSize);
    Push_WAVEFORMAT_Fmt           (wav, formatCode, channels, sampleRate, byteRate, blockAlign); // 14 bytes
    Push_PCMWAVEFORMAT_Fmt        (wav, containerSizeInBits);                                    //  2 bytes
    Push_WAVEFORMATEX_Fmt         (wav, cbSize);                                                 //  2 bytes
    Push_WAVEFORMATEXTENSIBLE_Fmt (wav, validBitDepth, channelMask, subformat);                  // 22 bytes
    // --------
    // 40 bytes
    // data
    PushSilentDataChunk (wav, dataChunkSize);

    return wav;
}

#endif

