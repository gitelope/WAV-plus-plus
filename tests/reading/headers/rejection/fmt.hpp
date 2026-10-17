// WAV++
// Copyright (c) 2025 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_fmt_hpp
#define wavplusplus_tests_fmt_hpp

#include <cstdint>
#include <vector>

inline void ValidBitsPerSampleEqualsZero (TestLog & log)
{
    constexpr uint16_t validBitsPerSample {0};
    constexpr uint16_t containerSize     {24};

    constexpr uint16_t channels {2};
    constexpr uint32_t sr {48000};
    constexpr uint32_t frames {8};

    const auto bytes = buildWAVEFORMATEXTENSIBLE (wav::ExtensibleFormat::LinearPCM, validBitsPerSample, containerSize, channels, sr, frames, wav::SpeakerConfiguration::Stereo);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, valid bits per sample equals zero, WAVEFORMATEXTENSIBLE", wav::ReadErrorCode::ValidBitsPerSampleZero);
}
inline void BlockAlignEqualsZero (TestLog & log)
{
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8);
    OverwriteUint16LE (bytes, chunkOffset ("fmt ", bytes) + 20, 0);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, block align zero", wav::ReadErrorCode::BlockAlignZero);
}
inline void BlockAlignNotDivisibleByChannels (TestLog & log)
{
    constexpr uint16_t channels {2};
    constexpr uint16_t bitDepth {32};
    constexpr uint16_t containerSize {bitDepth};

    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, bitDepth, containerSize, channels, 48000, 8); // blockAlign is 2 * 32/8 = 8
    // overwrite channels with 3
    OverwriteUint16LE (bytes, chunkOffset ("fmt ", bytes) + 10, 3);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, block align not divisible by channels", wav::ReadErrorCode::BlockAlignNotDivisibleByChannels);
}
inline void ChannelsEqualsZero (TestLog & log)
{
    constexpr uint16_t channels {0};
    const auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, channels, 44100, 8);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, zero channels", wav::ReadErrorCode::ChannelsZero);
}
inline void ChannelsExceedMax (TestLog & log)
{
    constexpr uint16_t channels {wav::maxChannels + 1};
    constexpr uint32_t frames {channels};
    const auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, channels, 44100, frames);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, channels exceed WAV++ maximum", wav::ReadErrorCode::ChannelsExceedsMax);
}
inline void BitsPerSampleChannelsBlockAlignMismatch (TestLog & log)
{
    // throw ReadError when bitsPerSample does not equal 8 * blockAlign / channels

    // PCMWAVEFORMAT, WAVEFORMATEX, and WAVEFORMATEXTENSIBLE
    // float and integer

    constexpr uint16_t channels {2};
    constexpr auto channelMask {wav::SpeakerConfiguration::Stereo}; // for WAVEFORMATEXTENSIBLE only

    constexpr uint32_t frames {8};

    constexpr size_t blockAlignOffset {20}; // from beginning of fmt chunk

    {
        constexpr uint16_t bitDepth {16};  // 2 bytes
        constexpr uint16_t blockAlign {8}; // should be 4 bytes: 2 bytes / channel * 2 channels

        constexpr uint32_t sampleRate {44100};
        {
            auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, bitDepth, bitDepth, channels, sampleRate, frames);
            OverwriteUint16LE (bytes, chunkOffset ("fmt ", bytes) + blockAlignOffset, blockAlign);
            TestForReadError (bytes, log, "Header Rejection - fmt chunk, bitsPerSample/channels/blockAlign mismatch, PCMWAVEFORMAT, linear PCM", wav::ReadErrorCode::BitsPerSampleChannelsBlockAlignMismatch);
        }
        {
            auto bytes = buildWAVEFORMATEX (wav::FormatCode::LinearPCM, bitDepth, bitDepth, channels, sampleRate, frames);
            OverwriteUint16LE (bytes, chunkOffset ("fmt ", bytes) + blockAlignOffset, blockAlign);
            TestForReadError (bytes, log, "Header Rejection - fmt chunk, bitsPerSample/channels/blockAlign mismatch, WAVEFORMATEX, linear PCM", wav::ReadErrorCode::BitsPerSampleChannelsBlockAlignMismatch);
        }
        {
            auto bytes = buildWAVEFORMATEXTENSIBLE (wav::ExtensibleFormat::LinearPCM, bitDepth, bitDepth, channels, sampleRate, frames, channelMask);
            OverwriteUint16LE (bytes, chunkOffset ("fmt ", bytes) + blockAlignOffset, blockAlign);
            TestForReadError (bytes, log, "Header Rejection - fmt chunk, bitsPerSample/channels/blockAlign mismatch, WAVEFORMATEXTENSIBLE, linear PCM", wav::ReadErrorCode::BitsPerSampleChannelsBlockAlignMismatch);
        }
    }

    {
        constexpr uint16_t bitDepth {32};   // 4 bytes
        constexpr uint16_t blockAlign {16}; // should be 8 bytes: 4 bytes / channel * 2 channels

        constexpr uint32_t sampleRate {48000};
        {

            auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::FloatPCM, bitDepth, bitDepth, channels, sampleRate, frames);
            OverwriteUint16LE (bytes, chunkOffset ("fmt ", bytes) + blockAlignOffset, blockAlign);
            TestForReadError (bytes, log, "Header Rejection - fmt chunk, bitsPerSample/channels/blockAlign mismatch, PCMWAVEFORMAT, float PCM", wav::ReadErrorCode::BitsPerSampleChannelsBlockAlignMismatch);
        }
        {
            auto bytes = buildWAVEFORMATEX (wav::FormatCode::FloatPCM, bitDepth, bitDepth, channels, sampleRate, frames);
            OverwriteUint16LE (bytes, chunkOffset ("fmt ", bytes) + blockAlignOffset, blockAlign);
            TestForReadError (bytes, log, "Header Rejection - fmt chunk, bitsPerSample/channels/blockAlign mismatch, WAVEFORMATEX, float PCM", wav::ReadErrorCode::BitsPerSampleChannelsBlockAlignMismatch);
        }
        {
            auto bytes = buildWAVEFORMATEXTENSIBLE (wav::ExtensibleFormat::FloatPCM, bitDepth, bitDepth, channels, sampleRate, frames, channelMask);
            OverwriteUint16LE (bytes, chunkOffset ("fmt ", bytes) + blockAlignOffset, blockAlign);
            TestForReadError (bytes, log, "Header Rejection - fmt chunk, bitsPerSample/channels/blockAlign mismatch, WAVEFORMATEXTENSIBLE, float PCM", wav::ReadErrorCode::BitsPerSampleChannelsBlockAlignMismatch);
        }
    }
}
inline void ExtensibleFmtSizeCodeMismatch (TestLog & log)
{
    // fmt size 40 requires format code 0xFFFE (Extensible).
    {
        auto bytes = buildWAVEFORMATEXTENSIBLE (wav::ExtensibleFormat::LinearPCM, 24, 24, 2, 48000, 8, wav::SpeakerConfiguration::Stereo);
        OverwriteUint16LE (bytes, chunkOffset ("fmt ", bytes) + 8, wav::FormatCode::LinearPCM);
        TestForReadError (bytes, log, "Header Rejection - fmt chunk, 40-byte fmt chunk size but non-extensible format code", wav::ReadErrorCode::ExtensibleFmtSizeCodeMismatch);
    }

    // format code 0xFFFE requires fmt size 40.
    {
        auto bytes = buildWAVEFORMATEX (wav::FormatCode::LinearPCM, 24, 24, 2, 48000, 8);
        OverwriteUint16LE (bytes, chunkOffset ("fmt ", bytes) + 8, wav::FormatCode::Extensible);
        TestForReadError (bytes, log, "Header Rejection - fmt chunk, extensible format code without 40-byte fmt chunk size", wav::ReadErrorCode::ExtensibleFmtSizeCodeMismatch);
    }
}
inline void ExtensibleSubformatNotSupported (TestLog & log)
{
    auto bytes = buildWAVEFORMATEXTENSIBLE (wav::ExtensibleFormat::LinearPCM,
                                            16, 16, 2, 44100, 8,
                                            wav::SpeakerConfiguration::Stereo);
    // subFormat GUID starts 32 bytes after "fmt " (after validBits + channelMask).
    OverwriteUint32LE (bytes, chunkOffset ("fmt ", bytes) + 32, 0xBADDC0DE);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, extensible subformat not supported",
                      wav::ReadErrorCode::ExtensibleSubformatNotSupported);
}
inline void ValidBitsPerSampleNotEqualToFloatContainerSize (TestLog & log)
{
    constexpr uint16_t validBitsPerSample {32};
    constexpr uint16_t containerSize      {64};

    constexpr uint16_t channels {2};
    constexpr uint32_t frames {8};
    constexpr uint32_t sr {48000};

    const auto bytes = buildWAVEFORMATEXTENSIBLE (wav::ExtensibleFormat::FloatPCM, validBitsPerSample, containerSize, channels, sr, frames, wav::SpeakerConfiguration::Stereo);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, valid bits per sample != float container size, WAVEFORMATEXTENSIBLE", wav::ReadErrorCode::ValidBitsPerSampleNotEqualToFloatContainerSize);
}
inline void InvalidCbSize (TestLog & log)
{
    // WAVEFORMATEX: cbSize must be 0 (format chunk size 18).
    {
        auto bytes = buildWAVEFORMATEX (wav::FormatCode::LinearPCM, 24, 24, 2, 48000, 8);
        OverwriteUint16LE (bytes, chunkOffset ("fmt ", bytes) + 24, 22);
        TestForReadError (bytes, log, "Header Rejection - fmt chunk, invalid cbSize, WAVEFORMATEX", wav::ReadErrorCode::InvalidCbSize);
    }
    // WAVEFORMATEXTENSIBLE: cbSize must be 22 (format chunk size 40).
    {
        auto bytes = buildWAVEFORMATEXTENSIBLE (wav::ExtensibleFormat::LinearPCM,
                                                24, 24, 2, 48000, 8,
                                                wav::SpeakerConfiguration::Stereo);
        OverwriteUint16LE (bytes, chunkOffset ("fmt ", bytes) + 24, 0);
        TestForReadError (bytes, log, "Header Rejection - fmt chunk, invalid cbSize, WAVEFORMATEXTENSIBLE", wav::ReadErrorCode::InvalidCbSize);
    }
}
inline void ValidBitsPerSampleExceedsLinearContainerSize (TestLog & log)
{
    constexpr uint16_t validBitsPerSample {24};
    constexpr uint16_t containerSize      {16};

    constexpr uint16_t channels {2};
    constexpr uint32_t sr {48000};
    constexpr uint32_t frames {8};

    const auto bytes = buildWAVEFORMATEXTENSIBLE (wav::ExtensibleFormat::LinearPCM, validBitsPerSample, containerSize, channels, sr, frames, wav::SpeakerConfiguration::Stereo);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, valid bits per sample exceeds linear container size, WAVEFORMATEXTENSIBLE", wav::ReadErrorCode::ValidBitsPerSampleExceedsLinearContainerSize);
}
inline void MissingFmtChunk (TestLog & log)
{
    // We skip the data chunk because it would trigger a data before fmt chunk error.
    std::vector<uint8_t> bytes;
    PushFourCC (bytes, "RIFF");
    PushUint32LE (bytes, 4); // "WAVE" only
    PushFourCC (bytes, "WAVE");
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, missing fmt chunk", wav::ReadErrorCode::MissingFmtChunk);
}
inline void MultipleFmtChunks (TestLog & log)
{
    const auto formatCode {wav::FormatCode::LinearPCM};
    const uint16_t bitDepth {8};
    const uint16_t containerSize {8};
    const uint16_t channels {1};
    const uint32_t frames {8};
    const uint32_t sampleRate {44100};

    const uint16_t blockAlign {channels * (containerSize / 8u)};
    const uint32_t byteRate {blockAlign * sampleRate};

    auto bytes = buildPCMWAVEFORMAT (formatCode, bitDepth, containerSize, channels, sampleRate, frames);

    // 2nd fmt chunk
    PushFmtHeader          (bytes, wav::FormatChunkSize::PCMWAVEFORMAT);
    Push_WAVEFORMAT_Fmt    (bytes, formatCode, channels, sampleRate, byteRate, blockAlign);
    Push_PCMWAVEFORMAT_Fmt (bytes, bitDepth);

    UpdateRIFFChunkSize (bytes);

    TestForReadError (bytes, log, "Header Rejection - fmt chunk, multiple fmt chunks", wav::ReadErrorCode::MultipleFmtChunks);
}
inline void SampleRateEqualsZero (TestLog & log)
{
    constexpr uint32_t sampleRate {0};
    const auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, sampleRate, 8);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, sample rate zero", wav::ReadErrorCode::SampleRateZero);
}
inline void UnsupportedFloatContainerSize (TestLog & log)
{
    // PCMWAVEFORMAT float
    // WAVEFORMATEX float
    // WAVEFORMATEXTENSIBLE float
    // expected: bitsPerSample = 32 or 64
    constexpr uint16_t bitsPerSample {48};
    {
        const auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::FloatPCM,
                                               bitsPerSample, bitsPerSample, 1, 48000, 8);
        TestForReadError (bytes, log, "Header Rejection - fmt chunk, float PCM, unsupported bits per sample, PCMWAVEFORMAT", wav::ReadErrorCode::UnsupportedFloatContainerSize);
    }
    {
        const auto bytes = buildWAVEFORMATEX (wav::FormatCode::FloatPCM,
                                              bitsPerSample, bitsPerSample, 1, 48000, 8);
        TestForReadError (bytes, log, "Header Rejection - fmt chunk, float PCM, unsupported bits per sample, WAVEFORMATEX", wav::ReadErrorCode::UnsupportedFloatContainerSize);
    }
    {
        const auto bytes = buildWAVEFORMATEXTENSIBLE (wav::ExtensibleFormat::FloatPCM,
                                                      bitsPerSample, bitsPerSample, 2, 48000, 8,
                                                      wav::SpeakerConfiguration::Stereo);
        TestForReadError (bytes, log, "Header Rejection - fmt chunk, float PCM, unsupported bits per sample, WAVEFORMATEXTENSIBLE", wav::ReadErrorCode::UnsupportedFloatContainerSize);
    }
}
inline void UnsupportedFmtChunkSize (TestLog & log)
{
    auto bytes = buildWAVEFORMATEX (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8); // fmt chunk size is 18
    // add a couple zero bytes
    bytes.push_back (0);
    bytes.push_back (0);
    OverwriteUint32LE (bytes, chunkOffset ("fmt ", bytes) + 4, 20u); // 20 bytes is accurate but unsupported
    UpdateRIFFChunkSize (bytes);

    TestForReadError (bytes, log, "Header Rejection - fmt chunk, unsupported format chunk size",
                      wav::ReadErrorCode::UnsupportedFmtChunkSize);
}
inline void UnsupportedFormatCode (TestLog & log)
{
    const uint16_t unsupportedFormatCode {0xBADD};
    auto bytes = buildPCMWAVEFORMAT (unsupportedFormatCode, 8, 8, 1, 44100, 8);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, unsupported format code", wav::ReadErrorCode::UnsupportedFormatCode);
}
inline void UnsupportedLPCMContainerSize (TestLog & log)
{
    constexpr uint16_t bitDepth {64};
    constexpr uint16_t containerSize {bitDepth};
    {
        const auto bytes = buildWAVEFORMAT (wav::FormatCode::LinearPCM, bitDepth, 2, 48000, 8);
        TestForReadError (bytes, log, "Header Rejection - fmt chunk, unsupported LPCM container size, WAVEFORMAT", wav::ReadErrorCode::UnsupportedLPCMContainerSize);
    }
    {
        const auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, bitDepth, containerSize, 2, 48000, 8);
        TestForReadError (bytes, log, "Header Rejection - fmt chunk, unsupported LPCM container size, PCMWAVEFORMAT", wav::ReadErrorCode::UnsupportedLPCMContainerSize);
    }
    {
        const auto bytes = buildWAVEFORMATEX (wav::FormatCode::LinearPCM, bitDepth, containerSize, 2, 48000, 8);
        TestForReadError (bytes, log, "Header Rejection - fmt chunk, unsupported LPCM container size, WAVEFORMATEX", wav::ReadErrorCode::UnsupportedLPCMContainerSize);
    }
    {
        const auto bytes = buildWAVEFORMATEXTENSIBLE (wav::ExtensibleFormat::LinearPCM,
                                                      bitDepth, containerSize, 2, 48000, 8,
                                                      wav::SpeakerConfiguration::Stereo);
        TestForReadError (bytes, log, "Header Rejection - fmt chunk, unsupported LPCM container size, WAVEFORMATEXTENSIBLE", wav::ReadErrorCode::UnsupportedLPCMContainerSize);
    }
}

inline void RunFormatChunkTests (TestLog & log)
{
    BitsPerSampleChannelsBlockAlignMismatch (log);
    BlockAlignEqualsZero (log);
    BlockAlignNotDivisibleByChannels (log);
    ChannelsEqualsZero (log);
    ChannelsExceedMax (log);
    ExtensibleFmtSizeCodeMismatch (log);
    ExtensibleSubformatNotSupported (log);
    InvalidCbSize (log);
    MissingFmtChunk (log);
    MultipleFmtChunks (log);
    SampleRateEqualsZero (log);
    UnsupportedFloatContainerSize (log);
    UnsupportedFmtChunkSize (log);
    UnsupportedFormatCode (log);
    UnsupportedLPCMContainerSize (log);
    ValidBitsPerSampleEqualsZero (log);
    ValidBitsPerSampleExceedsLinearContainerSize (log);
    ValidBitsPerSampleNotEqualToFloatContainerSize (log);
}

#endif
