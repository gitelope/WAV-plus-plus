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
    TestForReadError (bytes, log, "Header Rejection - RIFF chunk, wrong subformat (not WAVE)");
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
    // Append "fmt " but don't bump RIFF size to reflect its contribution to the file size:
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8);
    PushFourCC (bytes, "fmt ");
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, multiple fmt chunks (2nd \"fmt \" chunk not reflected in RIFF chunk size)");

    // Adjust RIFF size to account for the "fmt " appended to the end of the file:
    OverwriteUint32LE (bytes, 4, static_cast<uint32_t> (bytes.size() - 8));
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, multiple fmt chunks (2nd \"fmt \" chunk reflected in RIFF chunk size)");
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
    // Append "data" but don't bump RIFF size to reflect its contribution to the file size:
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8);
    PushFourCC (bytes, "data");
    TestForReadError (bytes, log, "Header Rejection - data chunk, multiple data chunks (2nd \"data\" chunk not reflected in RIFF chunk size)");

    // Adjust RIFF size to account for the "data" appended to the end of the file:
    OverwriteUint32LE (bytes, 4, static_cast<uint32_t> (bytes.size() - 8));
    TestForReadError (bytes, log, "Header Rejection - data chunk, multiple data chunks (2nd \"data\" chunk reflected in RIFF chunk size)");
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
    const auto filename {"test_fact_size_not_four.wav"};
    constexpr uint32_t frames {8};
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::FloatPCM, 32, 32, 2, 48000, frames);
    PushFactChunk (bytes, frames);
    // overwrite the fact size field, replacing 4 with 5
    OverwriteUint32LE (bytes, chunkOffset ("fact", bytes) + 4, 5);
    // update the RIFF chunk size to reflect the fact chunk size
    OverwriteUint32LE (bytes, 4, static_cast<uint32_t> (bytes.size() - 8));
    WriteBytesToFile (filename, bytes);

    std::string failures;
    try
    {
        wav::ChunkInfo info {filename};
        Compare (info.fact.present == false, "fact.present should be false after skipping", failures);
    }
    catch (wav::ReadError const & e)
    {
        failures = std::string {"wav::ReadError - "} + e.what();
    }
    const auto suffix {failures.empty() ? "" : ", failures: " + failures};
    log.Record (failures.empty(), std::format ("Header Rejection - fact chunk, fact with bad size is skipped{}", suffix));

    std::remove (filename);
}
inline void MultipleFactChunks (TestLog & log)
{
    constexpr uint32_t frames {8};
    constexpr uint16_t bitDepth {8};
    constexpr uint16_t containerSize {bitDepth};
    constexpr uint16_t channels {1};
    constexpr uint32_t sampleRate {44100};

    // first fact chunk is well-formed
    {
        // Extra "fact" added but not reflected in RIFF chunk size:
        // Construct RIFF, fmt, and data chunks:
        auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, bitDepth, containerSize, channels, sampleRate, frames);
        // First fact chunk:
        PushFactChunk (bytes, frames);
        // Update RIFF chunk size to reflect first fact chunk:
        OverwriteUint32LE (bytes, 4, static_cast<uint32_t> (bytes.size() - 8));
        // Append second "fact" but don't update RIFF chunk size:
        PushFourCC (bytes, "fact"); // Just the ID, but should be enough to trigger the exception.
        TestForReadError (bytes, log, "Header Rejection - fact chunk, multiple fact chunks (2nd \"fact\" chunk not reflected in RIFF chunk size)");

        // Update RIFF chunk size to reflect second fact chunk and test again:
        OverwriteUint32LE (bytes, 4, static_cast<uint32_t> (bytes.size() - 8));
        TestForReadError (bytes, log, "Header Rejection - fact chunk, multiple fact chunks (2nd \"fact\" chunk reflected in RIFF chunk size)");
    }

    // first fact chunk is malformed
    {
        auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, bitDepth, containerSize, channels, sampleRate, frames);
        // First fact chunk:
        PushFactChunk (bytes, frames);
        // First fact claims size is 5 though it should be 4.
        // (Should be skipped but marked as encountered with ChunkInfo::factChunkEncountered set to true.)
        OverwriteUint32LE (bytes, chunkOffset ("fact", bytes) + 4, 5);
        // Update RIFF chunk size to reflect first fact chunk:
        OverwriteUint32LE (bytes, 4, static_cast<uint32_t> (bytes.size() - 8));
        // Append second "fact" but don't update RIFF chunk size:
        PushFourCC (bytes, "fact");
        TestForReadError (bytes, log,
                          "Header Rejection - fact chunk, multiple fact chunks (first is malformed and 2nd \"fact\" not reflected in RIFF chunk size)");

        // Update RIFF chunk size to reflect second fact chunk and test again:
        OverwriteUint32LE (bytes, 4, static_cast<uint32_t> (bytes.size() - 8));
        TestForReadError (bytes, log, "Header Rejection - fact chunk, multiple fact chunks (first is malformed and 2nd \"fact\" reflected in RIFF chunk size)");
    }
}

inline void RunFactChunkTests (TestLog & log)
{
    FactChunkSizeNotFour (log);
    MultipleFactChunks (log);
}

// PEAK CHUNK TESTS ///////////////////////////////////////
inline void MultiplePeakChunks (TestLog & log)
{
    // Extra "PEAK" added but not reflected in RIFF chunk size:
    constexpr uint32_t frames {8};
    constexpr uint16_t channels {2};
    // Construct RIFF, fmt, and data chunks:
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, channels, 44100, frames);
    // Add first PEAK chunk:
    const std::vector<wav::Peak> peaks
    {
        {0,0}, // peak of 0 at frame 0 for channel 0
        {0,0}  // peak of 0 at frame 0 for channel 1
    };
    const uint32_t timestamp {0}; // seconds elapsed since Jan 1, 1970
    PushPeakChunk (bytes, peaks, timestamp);
    // Update RIFF chunk size to reflect first PEAK chunk:
    OverwriteUint32LE (bytes, 4, static_cast<uint32_t> (bytes.size() - 8));
    // Append second "PEAK" but don't update RIFF chunk size:
    PushFourCC (bytes, "PEAK");
    TestForReadError (bytes, log, "Header Rejection - PEAK chunk, multiple PEAK chunks (2nd \"PEAK\" chunk not reflected in RIFF chunk size)");

    // Update RIFF chunk size to reflect second PEAK chunk:
    OverwriteUint32LE (bytes, 4, static_cast<uint32_t> (bytes.size() - 8));
    TestForReadError (bytes, log, "Header Rejection - PEAK chunk, multiple PEAK chunks (2nd \"PEAK\" chunk reflected in RIFF chunk size)");
}

inline void RunPeakChunkTests (TestLog & log)
{
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

#endif
