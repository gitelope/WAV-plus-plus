
#ifndef wavplusplus_tests_Header_Tests_hpp
#define wavplusplus_tests_Header_Tests_hpp

#include "../../include/wav-plus-plus/Writer.hpp"
#include "../../include/wav-plus-plus/private/ReadingBytes.hpp"

#include <bit>
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
    // Verify the entire header.

    // WAVEFORMATEX when channels <= 2, channelMask == 0, and bitsPerSample <= 16.
    // WAVEFORMATEXTENSIBLE otherwise.

    // Each encoding is tested at mono, stereo, and 3+ channels.

    // Padding is tested for 8-bit and 24-bit—the only encodings that can produce
    // an odd block align (bytes per frame) since that requires byte depth, number
    // of frames, and number of channels all to be odd.

    // Speaker position and channel mask validation are tested separately.

    //          encoding                          sr       ch  frames  label
    //-----------------------------------------------------------------------------------
    //
    // unsigned 8-bit LPCM — WAVEFORMATEX for channels <= 2, WAVEFORMATEXTENSIBLE for 3+ channels
    TestHeader (wav::ExportFormat::LinearPCM_8,   22050,   1,  4,      "u8 mono",        log);
    TestHeader (wav::ExportFormat::LinearPCM_8,   22050,   1,  5,      "u8 mono (pad)",  log);
    TestHeader (wav::ExportFormat::LinearPCM_8,   22050,   2,  3,      "u8 stereo",      log);
    TestHeader (wav::ExportFormat::LinearPCM_8,   22050,   3,  5,      "u8 3ch (pad)",   log);
    //
    // signed 16-bit LPCM — WAVEFORMATEX for channels <= 2, WAVEFORMATEXTENSIBLE for 3+ channels
    TestHeader (wav::ExportFormat::LinearPCM_16,  44100,   1,  3,      "s16 mono",       log);
    TestHeader (wav::ExportFormat::LinearPCM_16,  44100,   2,  3,      "s16 stereo",     log);
    TestHeader (wav::ExportFormat::LinearPCM_16,  44100,   3,  5,      "s16 3ch",        log);
    //
    // signed 24-bit LPCM — WAVEFORMATEXTENSIBLE
    TestHeader (wav::ExportFormat::LinearPCM_24,  48000,   1,  7,      "s24 mono (pad)", log);
    TestHeader (wav::ExportFormat::LinearPCM_24,  48000,   2,  5,      "s24 stereo",     log);
    TestHeader (wav::ExportFormat::LinearPCM_24,  48000,   3,  3,      "s24 3ch (pad)",  log);
    //
    // signed 32-bit LPCM — WAVEFORMATEXTENSIBLE
    TestHeader (wav::ExportFormat::LinearPCM_32,  96000,   1,  5,      "s32 mono",       log);
    TestHeader (wav::ExportFormat::LinearPCM_32,  96000,   2,  5,      "s32 stereo",     log);
    TestHeader (wav::ExportFormat::LinearPCM_32,  96000,   3,  5,      "s32 3ch",        log);

    // 32-bit float PCM — WAVEFORMATEXTENSIBLE
    TestHeader (wav::ExportFormat::FloatPCM_32,   192000,  1,  3,      "f32 mono",       log);
    TestHeader (wav::ExportFormat::FloatPCM_32,   192000,  2,  3,      "f32 stereo",     log);
    TestHeader (wav::ExportFormat::FloatPCM_32,   192000,  5,  3,      "f32 5ch",        log);

    // 64-bit float PCM — WAVEFORMATEXTENSIBLE
    TestHeader (wav::ExportFormat::FloatPCM_64,   384000,  1,  5,      "f64 mono",       log);
    TestHeader (wav::ExportFormat::FloatPCM_64,   384000,  2,  5,      "f64 stereo",     log);
    TestHeader (wav::ExportFormat::FloatPCM_64,   384000,  4,  5,      "f64 4ch",        log);
}

/*

 WAV++ uses the following chunk order:

 RIFF
     fmt
     data

 WAVEFORMATEX is used (fmt chunk size = 18, header size = 46 bytes)
 when channels <= 2, channelMask == 0, and bitsPerSample <= 16.

 WAVEFORMATEXTENSIBLE is used (fmt chunk size = 40, header size = 68 bytes)
 when channels >= 3, channelMask != 0, or bitsPerSample > 16.

 (A header is here defined as everything from "RIFF" through the data chunk
 size that precedes the data portion of the data chunk.)

 File size (in bytes)
 = headerSize + dataChunkSize + pad*
 = riffChunkSize + 8

 * Pad is 1 if dataChunkSize is odd and 0 if even.
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
    {
        if (fmtChunkSize == wav::FormatChunkSize::WAVEFORMATEXTENSIBLE)
        {
            validBitsPerSample = wav::uint16_le (in);
            channelMask        = wav::uint32_le (in);
            subFormat          = wav::guid (in);
        }

        dataID        = wav::fourCharacterCode (in);
        dataChunkSize = wav::uint32_le (in);
    }

    // RIFF
    std::string riffID;
    uint32_t riffChunkSize;
    std::string riffFormat;

    // fmt subchunk — fields common to WAVEFORMATEX and WAVEFORMATEXTENSIBLE
    std::string fmtID;
    uint32_t fmtChunkSize;
    uint16_t formatCode;
    uint16_t channels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    uint16_t cbSize;

    // fmt subchunk — WAVEFORMATEXTENSIBLE-specific fields
    uint16_t validBitsPerSample {0xFFFF};
    uint32_t channelMask        {0xFFFFFFFF};
    wav::GUID subFormat         {};

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

    const auto [bitDepth, formatCode] = wav::bitDepthAndFormatCode (encoding);
    const bool useExtensible {channels >= 3 || bitDepth > 16};

    const auto expectedFmtChunkSize {useExtensible
        ? wav::FormatChunkSize::WAVEFORMATEXTENSIBLE
        : wav::FormatChunkSize::WAVEFORMATEX};

    const uint16_t expectedFormatCode {useExtensible
        ? wav::FormatCode::Extensible
        : formatCode};

    const auto bytesPerSample {bitDepth / 8U};
    const auto blockAlign     {channels * bytesPerSample};
    const auto byteRate       {sampleRate * blockAlign};
    const auto dataChunkSize  {frames * blockAlign};
    const auto riffChunkSize  {(20 + expectedFmtChunkSize) + dataChunkSize + (dataChunkSize % 2)};

    std::vector<float> zeros (frames * channels, 0.f);

    wav::Writer writer {filename, encoding, sampleRate, channels};
    writer.write (zeros);
    writer.Close();

    const auto h = ReadHeader (filename);

    std::string failures;

    Compare (h.riffID == "RIFF",                         "RIFF id", failures);
    Compare (h.riffChunkSize == riffChunkSize,           "RIFF chunk size", failures);
    Compare (h.riffFormat == "WAVE",                     "RIFF format (WAVE)", failures);

    Compare (h.fmtID == "fmt ",                          "fmt id", failures);
    Compare (h.fmtChunkSize == expectedFmtChunkSize,     "fmt chunk size", failures);
    Compare (h.formatCode == expectedFormatCode,         "format code", failures);
    Compare (h.channels == channels,                     "channels", failures);
    Compare (h.sampleRate == sampleRate,                 "sample rate", failures);
    Compare (h.byteRate == byteRate,                     "byte rate", failures);
    Compare (h.blockAlign == blockAlign,                 "block align", failures);
    Compare (h.bitsPerSample == bitDepth,                "bit depth", failures);

    if (useExtensible)
    {
        Compare (h.cbSize == 22,                         "cbSize", failures);
        Compare (h.validBitsPerSample == bitDepth,       "validBitsPerSample", failures);
        Compare (h.channelMask == 0,                     "channel mask", failures);

        const auto expectedGUID {formatCode == wav::FormatCode::LinearPCM
            ? wav::ExtensibleFormat::LinearPCM
            : wav::ExtensibleFormat::FloatPCM};
        Compare (h.subFormat == expectedGUID,            "SubFormat GUID", failures);
    }
    else
    {
        Compare (h.cbSize == 0,                          "cbSize", failures);
    }

    Compare (h.dataID == "data",                         "data id", failures);
    Compare (h.dataChunkSize == dataChunkSize,           "data chunk size", failures);

    const auto logMessage
    {"WRITE HEADER - " + label + (failures.empty() ? "" : " failures: " + failures)};

    log.Record (failures.empty(), logMessage);
}

void TestSpeakerPositionHeader (TestLog & log)
{
    constexpr std::string filename {"test.wav"};

    const uint32_t channelMask =
        wav::SpeakerPosition::FrontLeft    |
        wav::SpeakerPosition::FrontRight   |
        wav::SpeakerPosition::FrontCenter  |
        wav::SpeakerPosition::LowFrequency |
        wav::SpeakerPosition::BackLeft     |
        wav::SpeakerPosition::BackRight;

    const auto channels   {std::popcount (channelMask)};
    const auto encoding   {wav::ExportFormat::LinearPCM_24};
    const auto sampleRate {48000};
    const auto frames     {10};

    const auto [bitDepth, formatCode] = wav::bitDepthAndFormatCode (encoding);
    const auto bytesPerSample {bitDepth / 8U};
    const auto blockAlign     {channels * bytesPerSample};
    const auto byteRate       {sampleRate * blockAlign};
    const auto dataChunkSize  {frames * blockAlign};
    const auto riffChunkSize  {(20 + wav::FormatChunkSize::WAVEFORMATEXTENSIBLE) + dataChunkSize + (dataChunkSize % 2)};

    std::vector<float> zeros (frames * channels, 0.f);

    wav::Writer writer {filename, encoding, sampleRate, channels, channelMask};
    writer.write (zeros);
    writer.Close();

    const auto h = ReadHeader (filename);

    std::string failures;

    Compare (h.riffID == "RIFF",                                           "RIFF id", failures);
    Compare (h.riffChunkSize == riffChunkSize,                             "RIFF chunk size", failures);
    Compare (h.riffFormat == "WAVE",                                       "RIFF format (WAVE)", failures);

    Compare (h.fmtID == "fmt ",                                            "fmt id", failures);
    Compare (h.fmtChunkSize == wav::FormatChunkSize::WAVEFORMATEXTENSIBLE, "fmt chunk size", failures);
    Compare (h.formatCode == wav::FormatCode::Extensible,                  "format code", failures);
    Compare (h.channels == channels,                                       "channels", failures);
    Compare (h.sampleRate == sampleRate,                                   "sample rate", failures);
    Compare (h.byteRate == byteRate,                                       "byte rate", failures);
    Compare (h.blockAlign == blockAlign,                                   "block align", failures);
    Compare (h.bitsPerSample == bitDepth,                                  "bit depth", failures);
    Compare (h.cbSize == 22,                                               "cbSize", failures);
    Compare (h.validBitsPerSample == bitDepth,                             "validBitsPerSample", failures);
    Compare (h.channelMask == channelMask,                                 "channel mask", failures);
    Compare (h.subFormat == wav::ExtensibleFormat::LinearPCM,              "SubFormat GUID", failures);

    Compare (h.dataID == "data",                                           "data id", failures);
    Compare (h.dataChunkSize == dataChunkSize,                             "data chunk size", failures);

    const auto logMessage {"WRITE HEADER - speaker positions (5.1)"
        + std::string {failures.empty() ? "" : " failures: " + failures}};

    log.Record (failures.empty(), logMessage);
}

void TestSpeakerPositionValidation (TestLog & log)
{
    bool caughtInvalid {false};
    try
    {
        wav::Writer writer {"test.wav", wav::ExportFormat::LinearPCM_16, 44100, 1, 0x00080000};
    }
    catch (wav::WriteError const &)
    {
        caughtInvalid = true;
    }
    log.Record (caughtInvalid, "WRITE HEADER - invalid channel mask bits throws");
}

#endif

