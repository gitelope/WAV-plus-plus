// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_Rejection_hpp
#define wavplusplus_tests_Rejection_hpp

#include <limits>

void RunRiffChunkTests   (TestLog &);
void RunFormatChunkTests (TestLog &);
void RunDataChunkTests   (TestLog &);
void RunFactChunkTests   (TestLog &);
void RunPeakChunkTests   (TestLog &);

inline void TestRejection (TestLog & log)
{
    RunRiffChunkTests   (log);
    RunFormatChunkTests (log);
    RunDataChunkTests   (log);
    RunFactChunkTests   (log);
    RunPeakChunkTests   (log);
}

// HELPER FUNCTIONS ///////////////////////////////////////////////////////////////

void TestForReadError (std::vector<uint8_t> const &, TestLog &, std::string const &, std::optional<wav::ReadErrorCode> = std::nullopt);
void UpdateRIFFChunkSize (std::vector<uint8_t> &);

// RIFF CHUNK TESTS ///////////////////////////////////////////////////////////////

inline void FirstFourBytesNotRIFF (TestLog & log)
{
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8);
    bytes[0] = 'J'; bytes[1] = 'I'; bytes[2] = 'F'; bytes[3] = 'F'; // JIFF
    TestForReadError (bytes, log, "Header Rejection - RIFF chunk, first four bytes not RIFF");
}
inline void WrongRIFFSubformat (TestLog & log)
{
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8);
    bytes[8] = 'S'; bytes[9] = 'A'; bytes[10] = 'V'; bytes[11] = 'E'; // SAVE
    TestForReadError (bytes, log, "Header Rejection - RIFF chunk, subformat not WAVE");
}
inline void MultipleRIFFChunks (TestLog & log)
{
    // *** Test will fail if ReadFile becomes bounded by RIFF chunk size (instead of EOF). ***
    //
    // Duplicates RIFF chunk, but doesn't update the first RIFF chunk size so
    // 2nd RIFF chunk would never be seen in that case.
    //
    // A couple options if that happens:
    // a) Remove test.
    // b) Update first RIFF chunk size with UpdateRIFFChunkSize (bytes) to
    //    effectively embed the 2nd chunk in the first.
    // c) Replace whole 2nd RIFF chunk with the four-character code "RIFF" and
    //    update the first RIFF chunk size so only the 4CC is embedded/duplicated.
    //
    // It's probably also not a bad idea to add a separate test elsewhere that
    // nothing after RIFF boundary is read so maybe append a second fmt chunk, don't
    // update RIFF chunk size, and check that no duplicate-fmt-chunk ReadError is
    // thrown.

    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8);
    const auto duplicate {bytes};
    bytes.insert (bytes.end(), duplicate.begin(), duplicate.end());
    TestForReadError (bytes, log, "Header Rejection - RIFF chunk, multiple RIFF chunks");
}

inline void RunRiffChunkTests (TestLog & log)
{
    FirstFourBytesNotRIFF (log);
    WrongRIFFSubformat (log);
    MultipleRIFFChunks (log);
}

// FORMAT CHUNK TESTS ///////////////////////////////////////////////////////////////

/*
inline void BitsPerSampleZero (TestLog & log)
{
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8);
    OverwriteUint16LE (bytes, chunkOffset ("fmt ", bytes) + 22, 0);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, bits per sample zero", wav::ReadErrorCode::BitsPerSampleZero);
}
 */
/*
inline void BitsPerSampleOver64 (TestLog & log)
{
    constexpr uint16_t bitDepth {128};
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8);
    OverwriteUint16LE (bytes, chunkOffset ("fmt ", bytes) + 22, bitDepth);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, bits per sample exceeds max", wav::ReadErrorCode::BitsPerSampleExceedsSupportedMax);
}
 */
inline void ZeroChannels (TestLog & log)
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
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, channels exceed WAV++ maximum", wav::ReadErrorCode::ChannelsExceedMax);
}
inline void SampleRateZero (TestLog & log)
{
    constexpr uint32_t sampleRate {0};
    const auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, sampleRate, 8);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, sample rate zero", wav::ReadErrorCode::SampleRateZero);
}
inline void BlockAlignZero (TestLog & log)
{
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8);
    OverwriteUint16LE (bytes, chunkOffset ("fmt ", bytes) + 20, 0);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, block align zero", wav::ReadErrorCode::BlockAlignZero);
}
inline void UnsupportedFormatCode (TestLog & log)
{
    const uint16_t unsupportedFormatCode {0xBADD};
    auto bytes = buildPCMWAVEFORMAT (unsupportedFormatCode, 8, 8, 1, 44100, 8);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, unsupported format code", wav::ReadErrorCode::UnsupportedFormatCode);
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
inline void ExtensibleFormatSizeCodeMismatch (TestLog & log)
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
inline void InvalidFormatSize (TestLog & log)
{
    const uint32_t badSize {0xBADDBADD};
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8);
    OverwriteUint32LE (bytes, chunkOffset ("fmt ", bytes) + 4, badSize);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, unsupported format chunk size", wav::ReadErrorCode::UnsupportedFmtChunkSize);
}
inline void MultipleFormatChunks (TestLog & log)
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
inline void MissingFmt (TestLog & log)
{
    // As of 2026.06.17, adding a data chunk would trip a no fmt before data error,
    // so we skip the data chunk, too.
    std::vector<uint8_t> bytes;
    PushFourCC (bytes, "RIFF");
    PushUint32LE (bytes, 4); // "WAVE" only
    PushFourCC (bytes, "WAVE");
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, missing fmt chunk");
}
inline void UnsupportedFloatSampleWidth (TestLog & log)
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
inline void BitsPerSampleChannelsAndBlockAlignMismatch (TestLog & log)
{
    // PCMWAVEFORMAT float
    // WAVEFORMATEX float
    // WAVEFORMATEXTENSIBLE float and integer
    // expected: bitsPerSample = 8 * blockAlign / channels

    constexpr uint16_t channels {2};
    {
        auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::FloatPCM, 32, 32, channels, 48000, 8);
        OverwriteUint16LE (bytes, chunkOffset ("fmt ", bytes) + 20, 16); // correct is 8
        TestForReadError (bytes, log, "Header Rejection - fmt chunk, bitsPerSample/channels/blockAlign mismatch, PCMWAVEFORMAT float PCM", wav::ReadErrorCode::FloatBitsPerSampleChannelsBlockAlignMismatch);
    }
    {
        auto bytes = buildWAVEFORMATEX (wav::FormatCode::FloatPCM, 32, 32, channels, 48000, 8);
        OverwriteUint16LE (bytes, chunkOffset ("fmt ", bytes) + 20, 16); // correct is 8
        TestForReadError (bytes, log, "Header Rejection - fmt chunk, bitsPerSample/channels/blockAlign mismatch, WAVEFORMATEX float PCM", wav::ReadErrorCode::FloatBitsPerSampleChannelsBlockAlignMismatch);
    }

    const auto channelMask {wav::SpeakerConfiguration::Stereo};
    {
        auto bytes = buildWAVEFORMATEXTENSIBLE (wav::ExtensibleFormat::LinearPCM, 16, 16, channels, 44100, 8, channelMask);
        OverwriteUint16LE (bytes, chunkOffset ("fmt ", bytes) + 20, 8); // correct is 4
        TestForReadError (bytes, log, "Header Rejection - fmt chunk, bitsPerSample/channels/blockAlign mismatch, WAVEFORMATEXTENSIBLE, linear PCM", wav::ReadErrorCode::ExtensibleBitsPerSampleChannelsBlockAlignMismatch);
    }
    {
        auto bytes = buildWAVEFORMATEXTENSIBLE (wav::ExtensibleFormat::FloatPCM, 32, 32, channels, 48000, 8, channelMask);
        OverwriteUint16LE (bytes, chunkOffset ("fmt ", bytes) + 20, 16); // correct is 8
        TestForReadError (bytes, log, "Header Rejection - fmt chunk, bitsPerSample/channels/blockAlign mismatch, WAVEFORMATEXTENSIBLE, float PCM", wav::ReadErrorCode::ExtensibleBitsPerSampleChannelsBlockAlignMismatch);
    }
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
/*
inline void ExtensibleBitsPerSampleNotMultipleOfEight (TestLog & log)
{
    constexpr uint16_t validBitsPerSample {20};
    constexpr uint16_t bitsPerSample {24};

    // use LPCM since float PCM is further restricted to 32 and 64 bits
    constexpr auto subFormat {wav::ExtensibleFormat::LinearPCM};

    constexpr auto channelMask {wav::SpeakerConfiguration::Stereo};

    auto bytes = buildWAVEFORMATEXTENSIBLE (subFormat, validBitsPerSample, bitsPerSample, 2, 48000, 8, channelMask);
    // overwrite bitsPerSample with 28
    OverwriteUint16LE (bytes, chunkOffset ("fmt ", bytes) + 22, 28);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, WAVEFORMATEXTENSIBLE, bitsPerSample not multiple of 8", wav::ReadErrorCode::ExtensibleBitsPerSampleNotMultipleOf8);
}
 */

inline void RunFormatChunkTests (TestLog & log)
{
    // BitsPerSampleZero (log);
    // BitsPerSampleOver64 (log);
    ZeroChannels (log);
    ChannelsExceedMax (log);
    SampleRateZero (log);
    BlockAlignZero (log);
    UnsupportedFormatCode (log);
    ExtensibleFormatSizeCodeMismatch (log);
    InvalidCbSize (log);
    ExtensibleSubformatNotSupported (log);
    InvalidFormatSize (log);
    MultipleFormatChunks (log);
    MissingFmt (log);
    BitsPerSampleChannelsAndBlockAlignMismatch (log);
    BlockAlignNotDivisibleByChannels (log);
    UnsupportedFloatSampleWidth (log);
    UnsupportedLPCMContainerSize (log);
    // ExtensibleBitsPerSampleNotMultipleOfEight (log);
}

// DATA CHUNK TESTS ///////////////////////////////////////////////////////////////

inline void OverlargeDataChunkSize (TestLog & log)
{
    // Actual data chunk size:
    // 1 byte per sample x 1 channel x 8 frames = 8 bytes
    constexpr uint16_t bitDepth {8};
    constexpr uint16_t channels {1};
    constexpr uint32_t frames {8};
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, bitDepth, bitDepth, channels, 44100, frames);
    OverwriteUint32LE (bytes, chunkOffset ("data", bytes) + 4, frames + 1);
    TestForReadError (bytes, log, "Header Rejection - data chunk, purported size extends past actual EOF");
}
inline void DataChunkSizeNotDivisibleByBlockAlign (TestLog & log)
{
    // block align: 5 x 24 / 8 = 15 bytes
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 24, 24, 5, 44100, 1);
    OverwriteUint32LE (bytes, chunkOffset ("data", bytes) + 4, 16);
    TestForReadError (bytes, log, "Header Rejection - data chunk, size not divisible by block align");
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

    TestForReadError (bytes, log, "Header Rejection - data chunk, multiple data chunks");
}
inline void MissingData (TestLog & log)
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

    TestForReadError (bytes, log, "Header Rejection - data chunk, missing data chunk");
}
inline void DataChunkBeforeFormatChunk (TestLog & log)
{
    constexpr uint32_t frames {8};

    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, frames);

    const auto fmtChunkStart {chunkOffset ("fmt ", bytes)};
    std::vector<uint8_t> dataChunk;
    PushSilentDataChunk (dataChunk, frames); // pushing to dataChunk, not bytes
    bytes.insert (bytes.begin() + static_cast<std::ptrdiff_t> (fmtChunkStart), dataChunk.begin(), dataChunk.end());
    UpdateRIFFChunkSize (bytes);

    TestForReadError (bytes, log, "Header Rejection - data chunk, data before fmt");
}

inline void RunDataChunkTests (TestLog & log)
{
    OverlargeDataChunkSize (log);
    DataChunkSizeNotDivisibleByBlockAlign (log);
    MultipleDataChunks (log);
    MissingData (log);
    DataChunkBeforeFormatChunk (log);
}

// FACT CHUNK TESTS ///////////////////////////////////////////////////////////////

inline void FactChunkSizeNotFour (TestLog & log)
{
    constexpr uint32_t frames {8};
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::FloatPCM, 32, 32, 2, 48000, frames);
    const auto factChunkStart {bytes.size()};
    PushFactChunk (bytes, frames);
    // Rewrite fact chunk size to invalid value of 5.
    OverwriteUint32LE (bytes, factChunkStart + 4, 5);
    UpdateRIFFChunkSize (bytes);
    TestForReadError (bytes, log, "Header Rejection - fact chunk, size not 4");
}
inline void MultipleFactChunks (TestLog & log)
{
    constexpr uint32_t frames {8};
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, frames);
    PushFactChunk (bytes, frames);
    PushFactChunk (bytes, frames);
    UpdateRIFFChunkSize (bytes);
    TestForReadError (bytes, log, "Header Rejection - fact chunk, multiple fact chunks");
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
        TestForReadError (bytes, log, "Header Rejection - PEAK chunk, multiple PEAK chunks");
    }

    // First PEAK is version 2 (skipped); ReadError still thrown when 2nd PEAK chunk encountered
    {
        auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, channels, 44100, frames);
        const auto peakChunkStart {bytes.size()};
        PushPeakChunk (bytes, peaks, timestamp);
        OverwriteUint32LE (bytes, peakChunkStart + 8, 2); // Set version to 2.
        PushPeakChunk (bytes, peaks, timestamp);
        UpdateRIFFChunkSize (bytes);
        TestForReadError (bytes, log, "Header Rejection - PEAK chunk, multiple PEAK chunks (1st skipped due to unsupported version; 2nd throws ReadError)");
    }
}
inline void PeakChunkBeforeFmt (TestLog & log)
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

    TestForReadError (bytes, log, "Header Rejection - PEAK chunk, PEAK comes before fmt");
}
inline void OverlargePeakChunkSize (TestLog & log)
{
    constexpr uint16_t channels {2};

    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::FloatPCM, 32, 32, channels, 48000, 8);

    const auto peakChunkStart {bytes.size()};
    std::vector<wav::Peak> peaks;
    for (auto n = channels; n > 0; --n) peaks.push_back ({0.f, 0u});
    PushPeakChunk (bytes, peaks, 0);
    // version (4 bytes), timestamp (4 bytes), 8 bytes per peak (one peak per channel)
    constexpr uint32_t actualPeakSize {4u + 4u + 8u * channels};
    // Overwrite PEAK chunk size with size + 1.
    OverwriteUint32LE (bytes, peakChunkStart + 4, actualPeakSize + 1);
    UpdateRIFFChunkSize (bytes);

    TestForReadError (bytes, log, "Header Rejection - PEAK chunk, purported size extends past actual EOF");
}
inline void PeakChunkSizeMismatchesChannels (TestLog & log)
{
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::FloatPCM, 32, 32, 2, 48000, 8);
    // WAV has 2 channels but PEAK chunk size is not 8 + 8 * 2 = 24; it is 16.
    const std::vector<wav::Peak> peaks {{0.f, 0u}};
    PushPeakChunk (bytes, peaks, 0);
    UpdateRIFFChunkSize (bytes);

    TestForReadError (bytes, log, "Header Rejection - PEAK chunk, size mismatches channels");
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
inline void PeakChunkSizeTooSmallForVersion (TestLog & log)
{
    constexpr uint16_t channels {2};

    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::FloatPCM, 32, 32, channels, 48000, 8);

    const auto peakChunkStart {bytes.size()};
    std::vector<wav::Peak> peaks;
    for (auto n = channels; n > 0; --n) peaks.push_back ({0.f, 0u});
    PushPeakChunk (bytes, peaks, 0);
    OverwriteUint32LE (bytes, peakChunkStart + 4, 2); // version field needs 4 bytes in chunk body
    UpdateRIFFChunkSize (bytes);

    TestForReadError (bytes, log, "Header Rejection - PEAK chunk, size too small for version field");
}

inline void RunPeakChunkTests (TestLog & log)
{
    PeakChunkBeforeFmt (log);
    OverlargePeakChunkSize (log);
    PeakChunkSizeMismatchesChannels (log);
    PeakChunkVersionUnsupported (log);
    MultiplePeakChunks (log);
    PeakChunkSizeTooSmallForVersion (log);
}

// HELPER FUNCTIONS ///////////////////////////////////////////////////////////////

inline void TestForReadError (std::vector<uint8_t> const & bytes,
                              TestLog & log,
                              std::string const & label,
                              std::optional<wav::ReadErrorCode> expectedCode)
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
        if (expectedCode.has_value() && e.code() == *expectedCode) passes = true;
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
