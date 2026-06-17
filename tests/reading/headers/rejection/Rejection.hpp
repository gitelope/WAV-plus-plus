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

///////////////////////////////////////////////////////////

// HELPER FUNCTIONS ///////////////////////////////////////
void TestForReadError (std::vector<uint8_t> const &, TestLog &, std::string const &);
void UpdateRIFFChunkSize (std::vector<uint8_t> &);

// RIFF CHUNK TESTS ///////////////////////////////////////
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
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8);
    PushFourCC (bytes, "RIFF");
    TestForReadError (bytes, log, "Header Rejection - RIFF chunk, multiple RIFF chunks");
}

inline void RunRiffChunkTests (TestLog & log)
{
    FirstFourBytesNotRIFF (log);
    WrongRIFFSubformat (log);
    MultipleRIFFChunks (log);
}

// FORMAT CHUNK TESTS ///////////////////////////////////////
inline void BitsPerSampleZero (TestLog & log)
{
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8);
    OverwriteUint16LE (bytes, chunkOffset ("fmt ", bytes) + 22, 0);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, bits per sample zero");
}
inline void BitsPerSampleOver64 (TestLog & log)
{
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8);
    OverwriteUint16LE (bytes, chunkOffset ("fmt ", bytes) + 22, 65);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, bits per sample exceeds max");
}
inline void ZeroChannels (TestLog & log)
{
    constexpr uint16_t channels {0};
    const auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, channels, 44100, 8);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, zero channels");
}
inline void ChannelsExceedMax (TestLog & log)
{
    constexpr uint16_t channels {wav::maxChannels + 1};
    constexpr uint32_t frames {channels};
    const auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, channels, 44100, frames);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, channels exceed WAV++ maximum");
}
inline void SampleRateZero (TestLog & log)
{
    constexpr uint32_t sampleRate {0};
    const auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, sampleRate, 8);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, sample rate zero");
}
inline void BlockAlignZero (TestLog & log)
{
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8);
    OverwriteUint16LE (bytes, chunkOffset ("fmt ", bytes) + 20, 0);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, block align zero");
}
inline void UnsupportedFormatCode (TestLog & log)
{
    const uint16_t unsupportedFormatCode {0xBADD};
    auto bytes = buildPCMWAVEFORMAT (unsupportedFormatCode, 8, 8, 1, 44100, 8);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, unsupported format code");
}
inline void InvalidFormatSize (TestLog & log)
{
    const uint32_t badSize {0xBADDBADD};
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8);
    OverwriteUint32LE (bytes, chunkOffset ("fmt ", bytes) + 4, badSize);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, invalid format size");
}
inline void MultipleFormatChunks (TestLog & log)
{
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8);

    PushFourCC (bytes, "fmt ");
    UpdateRIFFChunkSize (bytes);

    TestForReadError (bytes, log, "Header Rejection - fmt chunk, multiple fmt chunks");
}

inline void RunFormatChunkTests (TestLog & log)
{
    BitsPerSampleZero (log);
    BitsPerSampleOver64 (log);
    ZeroChannels (log);
    ChannelsExceedMax (log);
    SampleRateZero (log);
    BlockAlignZero (log);
    UnsupportedFormatCode (log);
    InvalidFormatSize (log);
    MultipleFormatChunks (log);
}

// DATA CHUNK TESTS ///////////////////////////////////////
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
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8);
    PushFourCC (bytes, "data");
    UpdateRIFFChunkSize (bytes);
    TestForReadError (bytes, log, "Header Rejection - data chunk, multiple data chunks");
}

inline void RunDataChunkTests (TestLog & log)
{
    OverlargeDataChunkSize (log);
    DataChunkSizeNotDivisibleByBlockAlign (log);
    MultipleDataChunks (log);
}

// FACT CHUNK TESTS ///////////////////////////////////////
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
    // First fact chunk:
    PushFactChunk (bytes, frames);
    // Append second "fact". Just the ID should be enough to trigger the exception:
    PushFourCC (bytes, "fact");
    UpdateRIFFChunkSize (bytes);
    TestForReadError (bytes, log, "Header Rejection - fact chunk, multiple fact chunks");
}

inline void RunFactChunkTests (TestLog & log)
{
    FactChunkSizeNotFour (log);
    MultipleFactChunks (log);
}

// PEAK CHUNK TESTS ///////////////////////////////////////
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

inline void RunPeakChunkTests (TestLog & log)
{
    PeakChunkBeforeFmt (log);
    OverlargePeakChunkSize (log);
    PeakChunkSizeMismatchesChannels (log);
    PeakChunkVersionUnsupported (log);
    MultiplePeakChunks (log);
}

// HELPER FUNCTIONS ///////////////////////////////////////
inline void TestForReadError (std::vector<uint8_t> const & bytes, TestLog & log, std::string const & label)
{
    const auto filename {"test_rejection.wav"};
    WriteBytesToFile (filename, bytes);

    bool threwReadError {false};
    std::string errorMessage;
    try
    {
        wav::Reader reader {filename};
    }
    catch (wav::ReadError const & e)
    {
        threwReadError = true;
        errorMessage = e.what();
    }
    log.Record (threwReadError, std::format ("{} - \"{}\"", label, errorMessage));

    std::remove (filename);
}

inline void UpdateRIFFChunkSize (std::vector<uint8_t> & bytes)
{
    OverwriteUint32LE (bytes, 4, static_cast<uint32_t> (bytes.size() - 8));
}

#endif
