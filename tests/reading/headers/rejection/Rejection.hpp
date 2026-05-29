// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_Rejection_hpp
#define wavplusplus_tests_Rejection_hpp

#include <limits>

void RunRiffChunkTests   (TestLog &);
void RunFormatChunkTests (TestLog &);
void RunDataChunkTests   (TestLog &);

inline void TestRejection (TestLog & log)
{
    RunRiffChunkTests   (log);
    RunFormatChunkTests (log);
    RunDataChunkTests   (log);
}

///////////////////////////////////////////////////////////

// HELPER FUNCTIONS ///////////////////////////////////////
inline void overwriteUint16LE (uint16_t, size_t, std::vector<uint8_t> &);
inline void overwriteUint32LE (uint32_t, size_t, std::vector<uint8_t> &);
void TestForReadError (std::vector<uint8_t> const &, TestLog &, std::string const &);
size_t chunkOffset (const char[4], std::vector<uint8_t> const &);

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

inline void RunRiffChunkTests (TestLog & log)
{
    FirstFourBytesNotRIFF (log);
    WrongRIFFSubformat (log);
}

// FORMAT CHUNK TESTS ///////////////////////////////////////
inline void BitsPerSampleZero (TestLog & log)
{
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8);
    overwriteUint16LE (0, chunkOffset ("fmt ", bytes) + 22, bytes);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, bits per sample zero");
}
inline void BitsPerSampleOver64 (TestLog & log)
{
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8);
    overwriteUint16LE (65, chunkOffset ("fmt ", bytes) + 22, bytes);
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
    overwriteUint16LE (0, chunkOffset ("fmt ", bytes) + 20, bytes);
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
    overwriteUint32LE (badSize, chunkOffset ("fmt ", bytes) + 4, bytes);
    TestForReadError (bytes, log, "Header Rejection - fmt chunk, invalid format size");
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
    overwriteUint32LE (frames + 1, chunkOffset ("data", bytes) + 4, bytes);
    TestForReadError (bytes, log, "Header Rejection - data chunk, purported size extends past actual EOF");
}
inline void DataChunkSizeNotDivisibleByBlockAlign (TestLog & log)
{
    // block align: 5 x 24 / 8 = 15 bytes
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 24, 24, 5, 44100, 1);
    overwriteUint32LE (16, chunkOffset ("data", bytes) + 4, bytes);
    TestForReadError (bytes, log, "Header Rejection - data chunk, size not divisible by block align");
}

inline void RunDataChunkTests (TestLog & log)
{
    OverlargeDataChunkSize (log);
    DataChunkSizeNotDivisibleByBlockAlign (log);
}

// HELPER FUNCTIONS ///////////////////////////////////////
inline void overwriteUint16LE (uint16_t value, size_t index, std::vector<uint8_t> & v)
{
    v[index]     = static_cast<uint8_t> (value     );
    v[index + 1] = static_cast<uint8_t> (value >> 8);
}

inline void overwriteUint32LE (uint32_t value, size_t index, std::vector<uint8_t> & v)
{
    v[index]     = static_cast<uint8_t> (value      );
    v[index + 1] = static_cast<uint8_t> (value >>  8);
    v[index + 2] = static_cast<uint8_t> (value >> 16);
    v[index + 3] = static_cast<uint8_t> (value >> 24);
}

inline void TestForReadError (std::vector<uint8_t> const & bytes, TestLog & log, std::string const & label)
{
    const auto filename {"test_rejection.wav"};
    writeFile (filename, bytes);

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

inline size_t chunkOffset (const char id[4], std::vector<uint8_t> const & bytes)
{
    for (size_t i = 0; i < bytes.size(); ++i)
    {
        if (bytes[i] == id[0] && bytes[i+1] == id[1] && bytes[i+2] == id[2] && bytes[i+3] == id[3])
            return i;
    }
    throw std::runtime_error (std::format ("Didn't find chunk: {}", id));
}

#endif
