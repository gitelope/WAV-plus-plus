
#ifndef wavplusplus_tests_ReadingHeaders_Tests_hpp
#define wavplusplus_tests_ReadingHeaders_Tests_hpp

#include "../../include/wav-plus-plus/Reader.hpp"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

void TestWAVEFORMAT (TestLog & log);

//////////////////////////////////////
void TestHeaderParsing (TestLog & log)
{
    TestWAVEFORMAT (log);
}
//////////////////////////////////////

std::vector<uint8_t> buildWAVEFORMAT (uint16_t, uint16_t, uint32_t, uint16_t, uint32_t);
void writeFile (std::string const &, std::vector<uint8_t> const &);

void TestWAVEFORMAT (TestLog & log)
{
    constexpr std::string filename {"test.wav"};
    constexpr uint16_t channels    {2};
    constexpr uint32_t sampleRate  {44100};
    constexpr uint32_t frames      {10};

    struct TestCase
    {
        uint16_t     formatCode;
        uint16_t     bitDepth;
        bool         expectLinearPCM;
        bool         expectFloatPCM;
        const char * label;
    };

    const TestCase tests[]
    {
        {wav::FormatCode::LinearPCM, 8,  true,  false, "WAVEFORMAT u8"},
        {wav::FormatCode::LinearPCM, 16, true,  false, "WAVEFORMAT s16"},
        {wav::FormatCode::LinearPCM, 24, true,  false, "WAVEFORMAT s24"},
        {wav::FormatCode::LinearPCM, 32, true,  false, "WAVEFORMAT s32"},
        {wav::FormatCode::FloatPCM,  32, false, true,  "WAVEFORMAT f32"},
        {wav::FormatCode::FloatPCM,  64, false, true,  "WAVEFORMAT f64"},
    };

    for (auto const & test : tests)
    {
        std::string failures;

        const auto bytes = buildWAVEFORMAT (test.formatCode, channels, sampleRate, test.bitDepth, frames);
        writeFile (filename, bytes);

        try
        {
            wav::Reader reader {filename};

            Compare (reader.channels()            == channels,             "channels",            failures);
            Compare (reader.sampleRate()          == sampleRate,           "sampleRate",          failures);
            Compare (reader.frames()              == frames,               "frames",              failures);
            Compare (reader.bitDepth()            == test.bitDepth,        "bitDepth",            failures);
            Compare (reader.containerSizeInBits() == test.bitDepth,        "containerSizeInBits", failures);
            Compare (reader.isLinearPCM()         == test.expectLinearPCM, "isLinearPCM",         failures);
            Compare (reader.isFloatPCM()          == test.expectFloatPCM,  "isFloatPCM",          failures);
        }
        catch (std::exception const & e)
        {
            failures = std::string {"exception: "} + e.what();
        }

        const std::string message {std::string {"Read Header - "} + test.label
            + (failures.empty() ? "" : " failures: " + failures)};

        log.Record (failures.empty(), message);
    }

    std::remove (filename.c_str());
}

void push_uint16_le (std::vector<uint8_t> &, uint16_t);
void push_uint32_le (std::vector<uint8_t> &, uint32_t);
void push_fourCC    (std::vector<uint8_t> &, const char *);

// Build a WAVEFORMAT WAV file (14-byte fmt chunk).
//
// Layout:
//   RIFF header   (12 bytes)
//   fmt  chunk    (8 + 14 = 22 bytes)
//   data chunk    (8 + dataSize bytes)
//   padding byte  (if dataSize is odd)
//
std::vector<uint8_t> buildWAVEFORMAT (uint16_t formatCode,
                                      uint16_t channels,
                                      uint32_t sampleRate,
                                      uint16_t bitDepth,
                                      uint32_t frames)
{
    assert (bitDepth % 8 == 0);
    const auto bytesPerSample {static_cast<uint16_t> (bitDepth / 8)};
    const auto blockAlign {static_cast<uint16_t> (channels * bytesPerSample)};
    const auto byteRate {sampleRate * blockAlign};
    const auto dataChunkSize {frames * blockAlign};
    const auto riffChunkSize {4 + (8 + 14) + (8 + dataChunkSize) + (dataChunkSize % 2)};

    std::vector<uint8_t> wav;

    // RIFF header
    push_fourCC    (wav, "RIFF");
    push_uint32_le (wav, riffChunkSize);
    push_fourCC    (wav, "WAVE");

    // fmt chunk (14 bytes)
    push_fourCC    (wav, "fmt ");
    push_uint32_le (wav, 14);
    push_uint16_le (wav, formatCode);
    push_uint16_le (wav, channels);
    push_uint32_le (wav, sampleRate);
    push_uint32_le (wav, byteRate);
    push_uint16_le (wav, blockAlign);

    // data chunk header
    push_fourCC    (wav, "data");
    push_uint32_le (wav, dataChunkSize);

    // insert silence into data chunk
    wav.resize (wav.size() + dataChunkSize, 0);

    // padding byte if odd
    if (dataChunkSize % 2) wav.push_back (0);

    return wav;
}

// Write byte vector to a file.
void writeFile (std::string const & filepath, std::vector<uint8_t> const & bytes)
{
    auto out = std::unique_ptr<FILE, decltype(&fclose)> (fopen (filepath.c_str(), "wb"), &fclose);

    if (!out)
    {
        throw std::runtime_error ("Failed to open file for writing: " + filepath);
    }

    if (fwrite (bytes.data(), 1, bytes.size(), out.get()) != bytes.size())
    {
        throw std::runtime_error ("Failed to write file: " + filepath);
    }
}

// Append a little-endian uint16 to byte vector.
void push_uint16_le (std::vector<uint8_t> & v, uint16_t value)
{
    v.push_back (static_cast<uint8_t> (value     ));
    v.push_back (static_cast<uint8_t> (value >> 8));
}

// Append a little-endian uint32 to byte vector.
void push_uint32_le (std::vector<uint8_t> & v, uint32_t value)
{
    v.push_back (static_cast<uint8_t> (value      ));
    v.push_back (static_cast<uint8_t> (value >> 8 ));
    v.push_back (static_cast<uint8_t> (value >> 16));
    v.push_back (static_cast<uint8_t> (value >> 24));
}

// Append a 4-character string to a byte vector.
void push_fourCC (std::vector<uint8_t> & v, const char * fourCC)
{
    v.push_back (static_cast<uint8_t> (fourCC[0]));
    v.push_back (static_cast<uint8_t> (fourCC[1]));
    v.push_back (static_cast<uint8_t> (fourCC[2]));
    v.push_back (static_cast<uint8_t> (fourCC[3]));
}

#endif

