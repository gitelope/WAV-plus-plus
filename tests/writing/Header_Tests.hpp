
#ifndef wavplusplus_tests_Header_Tests_hpp
#define wavplusplus_tests_Header_Tests_hpp

#include "../../include/wav-plus-plus/Writer.hpp"
#include "../../include/wav-plus-plus/private/ReadingBytes.hpp"

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

void TestHeader (wav::ExportFormat,
                 uint32_t sampleRate,
                 uint16_t channels,
                 uint32_t frames,
                 std::string const & label,
                 TestLog &);

void TestHeaders (TestLog & log)
{
    //          encoding                          sr       channels  frames  label
    TestHeader (wav::ExportFormat::LinearPCM_8,   22500,   3,        5,      "unsigned 8-bit LPCM", log); // padding byte
    TestHeader (wav::ExportFormat::LinearPCM_16,  44100,   2,        3,      "signed 16-bit LPCM",  log);
    TestHeader (wav::ExportFormat::LinearPCM_24,  48000,   1,        7,      "signed 24-bit LPCM",  log); // padding byte
    TestHeader (wav::ExportFormat::LinearPCM_32,  96000,   3,        5,      "signed 32-bit LPCM",  log);
    TestHeader (wav::ExportFormat::FloatPCM_32,   192000,  5,        3,      "32-bit float PCM",    log);
    TestHeader (wav::ExportFormat::FloatPCM_64,   384000,  4,        5,      "64-bit float PCM",    log);
}

/*

 WAV++ uses WAVEFORMATEX (fmt chunk size = 18)
 with the following chunk order:

 RIFF
     fmt
     data

 Headers will always be 46 bytes long and are defined as
 everything from "RIFF" through the data chunk size that
 precedes the data portion of the data chunk.

 File size (in bytes) = 46 + dataChunkSize + pad,
 where pad is 1 if dataChunkSize is odd and 0 if even

 File size (in bytes) = riffChunkSize + 8

*/

struct Header
{
    explicit Header (FILE * in)

    :riffID        (wav::fourCharacterCode (in))
    ,riffChunkSize (wav::uint32_le (in))
    ,riffFormat    (wav::fourCharacterCode (in))

    ,fmtID         (wav::fourCharacterCode (in))
    ,fmtChunkSize  (wav::uint32_le (in))
    ,formatCode    (wav::uint16_le (in))
    ,channels      (wav::uint16_le (in))
    ,sampleRate    (wav::uint32_le (in))
    ,byteRate      (wav::uint32_le (in))
    ,blockAlign    (wav::uint16_le (in))
    ,bitsPerSample (wav::uint16_le (in))
    ,cbSize        (wav::uint16_le (in))

    ,dataID        (wav::fourCharacterCode (in))
    ,dataChunkSize (wav::uint32_le (in))
    {}

    // RIFF
    std::string riffID;
    uint32_t riffChunkSize;
    std::string riffFormat;

    // fmt subchunk
    std::string fmtID;
    uint32_t fmtChunkSize;
    uint16_t formatCode;
    uint16_t channels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    uint16_t cbSize;

    // data subchunk (does not include the actual data)
    std::string dataID;
    uint32_t dataChunkSize;
};

Header ReadHeader (std::string const & filepath)
{
    auto in = wav::fileForReading (filepath);
    const Header h {in.get()};
    wav::Close (in);
    return h;
}

void Compare (bool passes, std::string const & failure, std::string & failures)
{
    if (!passes)
    {
        if (!failures.empty()) failures += ", ";
        failures += failure;
    }
}

void TestHeader (wav::ExportFormat encoding,
                 uint32_t sampleRate,
                 uint16_t channels,
                 uint32_t frames,
                 std::string const & label,
                 TestLog & log)
{
    constexpr std::string filename {"test.wav"};
    constexpr auto formatChunkSize {wav::FormatChunkSize::WAVEFORMATEX};

    const auto [bitDepth, formatCode] = wav::bitDepthAndFormatCode (encoding);
    const uint16_t bytesPerSample {static_cast<uint16_t> (bitDepth / 8)};
    const uint16_t blockAlign     {static_cast<uint16_t> (channels * bytesPerSample)};
    const uint32_t byteRate       {sampleRate * blockAlign};
    const uint32_t dataChunkSize  {frames * blockAlign};
    const uint32_t riffChunkSize  {38 + dataChunkSize + (dataChunkSize % 2)};

    std::vector<float> zeros (frames * channels, 0.f);

    wav::Writer writer {filename, encoding, sampleRate, channels};
    writer.write (zeros);
    writer.Close();

    const auto h = ReadHeader (filename);

    std::string failures;

    Compare (h.riffID == "RIFF",                "RIFF id", failures);
    Compare (h.riffChunkSize == riffChunkSize,  "RIFF chunk size", failures);
    Compare (h.riffFormat == "WAVE",            "RIFF format (WAVE)", failures);

    Compare (h.fmtID == "fmt ",                 "fmt id", failures);
    Compare (h.fmtChunkSize == formatChunkSize, "fmt chunk size", failures);
    Compare (h.formatCode == formatCode,        "format code", failures);
    Compare (h.channels == channels,            "channels", failures);
    Compare (h.sampleRate == sampleRate,        "sample rate", failures);
    Compare (h.byteRate == byteRate,            "byte rate", failures);
    Compare (h.blockAlign == blockAlign,        "block align", failures);
    Compare (h.bitsPerSample == bitDepth,       "bit depth", failures);
    Compare (h.cbSize == 0,                     "cbSize", failures);

    Compare (h.dataChunkSize == dataChunkSize,  "data chunk size", failures);
    Compare (h.dataID == "data",                "data id", failures);

    const auto logMessage
    {"WRITE HEADER - " + label + (failures.empty() ? "" : " failures: " + failures)};

    log.Record (failures.empty(), logMessage);
}

#endif

