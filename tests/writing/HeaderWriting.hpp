// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_Header_Tests_hpp
#define wavplusplus_tests_Header_Tests_hpp

#include "../../include/wav-plus-plus/Writer.hpp"
#include "../../include/wav-plus-plus/private/ReadingBytes.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

void TestHeader (wav::ExportFormat, uint32_t, uint16_t, uint32_t, TestLog &);
void TestSpeakerPosition (TestLog &);
void TestSpeakerPositionValidation (TestLog &);

void TestHeaders (TestLog & log)
{
    // Verify the entire header.
    //
    // WAVEFORMATEX when channels <= 2, channelMask == 0, and bitsPerSample <= 16.
    // WAVEFORMATEXTENSIBLE otherwise:
    //
    // unsigned 8-bit LPCM: WAVEFORMATEX for channels <= 2, WAVEFORMATEXTENSIBLE for 3+ channels
    // signed 16-bit LPCM:  WAVEFORMATEX for channels <= 2, WAVEFORMATEXTENSIBLE for 3+ channels
    // signed 24-bit LPCM:  WAVEFORMATEXTENSIBLE
    // signed 32-bit LPCM:  WAVEFORMATEXTENSIBLE
    // 32-bit float PCM:    WAVEFORMATEXTENSIBLE
    // 64-bit float PCM:    WAVEFORMATEXTENSIBLE
    //
    // Padding is tested for 8-bit and 24-bit—the only encodings that can produce
    // an odd number of data bytes since that requires bytes per sample,
    // channel count, and frame count to all be odd.
    //
    // Speaker position and channel mask validation are tested separately.

    constexpr auto maxChannels {4};

    constexpr auto frames {5};
    constexpr auto sampleRate {48000};

    constexpr std::array<wav::ExportFormat, 6> exportFormats
    {{
        wav::ExportFormat::LinearPCM_8,
        wav::ExportFormat::LinearPCM_16,
        wav::ExportFormat::LinearPCM_24,
        wav::ExportFormat::LinearPCM_32,
        wav::ExportFormat::FloatPCM_32,
        wav::ExportFormat::FloatPCM_64,
    }};

    for (auto const exportFormat : exportFormats)
    {
        for (uint16_t channels = 1; channels <= maxChannels; ++channels)
        {
            TestHeader (exportFormat, sampleRate, channels, frames, log);
        }
    }

    TestSpeakerPosition (log);
    TestSpeakerPositionValidation (log);
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
    explicit Header (std::FILE * in)

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

std::string headerTestLabel (wav::ExportFormat, uint16_t, uint32_t);
Header ReadHeader (std::string const &);

void TestHeader (wav::ExportFormat encoding,
                 uint32_t sampleRate,
                 uint16_t channels,
                 uint32_t frames,
                 TestLog & log)
{
    const auto filename {"testHeader.wav"};

    std::string const label = headerTestLabel (encoding, channels, frames);

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
    {"Write Header - " + label + (failures.empty() ? "" : " failures: " + failures)};

    log.Record (failures.empty(), logMessage);

    std::remove (filename);
}

void TestSpeakerPosition (TestLog & log)
{
    const auto filename       {"testSpeakerPosition.wav"};
    constexpr auto encoding   {wav::ExportFormat::LinearPCM_16};
    constexpr auto sampleRate {48000};
    constexpr auto frames     {10};

    struct TestCase
    {
        const char * label;
        uint32_t channelMask;
        uint16_t channels;
    };

    // All cases result in a WAVEFORMATEXTENSIBLE header (with a channelMask field).
    //
    // Recall that WAVEFORMATEXTENSIBLE is required for non-zero mask, 3+ channels, or bit depth > 16.
    // LPCM 16 was chosen because the bit depth does not require the WAVEFORMATEXTENSIBLE format.
    //
    // The stereo case has a non-zero mask.
    // The unassigned speakers case has 3+ channels.
    // The quadraphonic, 5.1, and 7.1 cases have both a non-zero mask and 3+ channels.
    const std::vector<TestCase> testCases
    {
        {"stereo",                wav::SpeakerConfiguration::Stereo,          2},
        {"3 unassigned channels", wav::SpeakerConfiguration::Unassigned,      3},
        {"quadraphonic",          wav::SpeakerConfiguration::Quadraphonic,    4},
        {"5.1",                   wav::SpeakerConfiguration::Five_Point_One,  6},
        {"7.1",                   wav::SpeakerConfiguration::Seven_Point_One, 8},
    };

    const auto [bitDepth, _] = wav::bitDepthAndFormatCode (encoding);

    for (auto const & testCase : testCases)
    {
        std::vector<float> zeros (static_cast<size_t> (frames) * testCase.channels, 0.f);

        wav::Writer writer {filename, encoding, sampleRate, testCase.channels, testCase.channelMask};
        writer.write (zeros);
        writer.Close();

        const auto h = ReadHeader (filename);

        std::string failures;

        Compare (h.fmtChunkSize == wav::FormatChunkSize::WAVEFORMATEXTENSIBLE, "fmt chunk size", failures);
        Compare (h.formatCode == wav::FormatCode::Extensible,                  "format code", failures);
        Compare (h.cbSize == 22,                                               "cbSize", failures);
        Compare (h.validBitsPerSample == bitDepth,                             "validBitsPerSample", failures);
        Compare (h.channelMask == testCase.channelMask,                        "channel mask", failures);
        Compare (h.subFormat == wav::ExtensibleFormat::LinearPCM,              "subFormat GUID", failures);

        const auto message = std::string {"Write Header - channel mask - "}
            + testCase.label
            + (failures.empty() ? "" : " failures: " + failures);

        log.Record (failures.empty(), message);

        std::remove (filename);
    }
}

void TestSpeakerPositionValidation (TestLog & log)
{
    const auto filename {"testSpeakerPositionValidation.wav"};

    bool caughtInvalid {false};
    try
    {
        constexpr uint32_t invalidMask {0xFFFFFFFF};
        wav::Writer writer {filename, wav::ExportFormat::LinearPCM_16, 44100, 1, invalidMask};
    }
    catch (wav::WriteError const &)
    {
        caughtInvalid = true;
    }
    log.Record (caughtInvalid, "Write Header - invalid channel mask bits throws");

    std::remove (filename);
}

Header ReadHeader (std::string const & filepath)
{
    auto in = wav::fileForReading (filepath);
    const Header h {in.get()};
    return h;
}

std::string headerTestLabel (wav::ExportFormat encoding, uint16_t channels, uint32_t frames)
{
    const auto [bitDepth, _] = wav::bitDepthAndFormatCode (encoding);
    const auto bytesPerSample {bitDepth / 8U};

    std::string label {exportFormatShortPrefix (encoding)};
    label += ", " + std::to_string (static_cast<unsigned> (channels)) + "ch";
    if (channels >= 3 || bitDepth > 16) label += ", WAVEFORMATEXTENSIBLE";
    else label += ", WAVEFORMATEX";
    if ((frames * channels * bytesPerSample) % 2) label += " (padded)";
    return label;
}

#endif

