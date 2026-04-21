
#ifndef wavplusplus_tests_SampleDecoding_Tests_hpp
#define wavplusplus_tests_SampleDecoding_Tests_hpp

#include "../reading_helper_functions.hpp"

#include <bit>
#include <cassert>
#include <cstdint>
#include <vector>

std::vector<uint8_t> buildWAVEFORMATEX (wav::ExportFormat, uint16_t, uint32_t, std::vector<uint8_t> const &);

void TestSampleDecoding (TestLog & log)
{
    constexpr auto sampleRate {44100};
    constexpr auto channels {1};
    const std::string filename {"testSampleDecoding.wav"};

    struct TestCase
    {
        wav::ExportFormat encoding;
        std::vector<uint8_t> data;
        std::vector<double> expected;
    };

    std::vector<TestCase> testCases
    {
        {
            wav::ExportFormat::LinearPCM_8,
            {
                0x00, //   0
                0x7F, // 127
                0x80, // 128
                0x81, // 129
                0xFF, // 255
            },
            {-1.0, -1/128., 0., 1/128., 127/128.}
        },
        {
            wav::ExportFormat::LinearPCM_16,
            {
                0x00, 0x80, // -32768
                0xFF, 0xFF, //     -1
                0x00, 0x00, //      0
                0x01, 0x00, //     +1
                0xFF, 0x7F, // +32767
            },
            {-1., -1/32768., 0., 1/32768., 32767/32768.}
        },
        {
            wav::ExportFormat::LinearPCM_24,
            {
                0x00, 0x00, 0x80, // -8388608
                0xFF, 0xFF, 0xFF, //       -1
                0x00, 0x00, 0x00, //        0
                0x01, 0x00, 0x00, //       +1
                0xFF, 0xFF, 0x7F, // +8388607
            },
            {-1., -1/8388608., 0., 1/8388608., 8388607/8388608.}
        },
        {
            wav::ExportFormat::LinearPCM_32,
            {
                0x00, 0x00, 0x00, 0x80, // -2147483648
                0xFF, 0xFF, 0xFF, 0xFF, //          -1
                0x00, 0x00, 0x00, 0x00, //           0
                0x01, 0x00, 0x00, 0x00, //          +1
                0xFF, 0xFF, 0xFF, 0x7F, // +2147483647
            },
            {-1., -1/2147483648., 0., 1/2147483648., 2147483647/2147483648.}
        },
        {
            wav::ExportFormat::FloatPCM_32,
            {
                0x00, 0x00, 0x80, 0xBF, // -1
                0x00, 0x00, 0x00, 0xBF, // -0.5
                0x00, 0x00, 0x00, 0x00, //  0
                0x00, 0x00, 0x00, 0x3F, // +0.5
                0x00, 0x00, 0x80, 0x3F, // +1
            },
            {-1., -0.5, 0., 0.5, 1.}
        },
        {
            wav::ExportFormat::FloatPCM_64,
            {
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xBF, // -1
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xBF, // -0.5
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //  0
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x3F, // +0.5
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, // +1
            },
            {-1., -0.5, 0., 0.5, 1.}
        },

    };
    for (auto & testCase : testCases)
    {
        writeFile (filename, buildWAVEFORMATEX (testCase.encoding, channels, sampleRate, testCase.data));
        {
            // read into float
            // (first convert expected from double to float)
            std::vector<float> expected_float (testCase.expected.size());
            for (size_t i = 0; i < testCase.expected.size(); ++i)
            {
                expected_float [i] = static_cast<float> (testCase.expected [i]);
            }
            wav::Reader reader {filename};
            std::vector<float> actual (reader.samples());
            const auto samplesRead = reader.read (actual);
            log.Record (samplesRead == expected_float.size() && areEqual (actual, expected_float), "Sample Decoding - " + exportFormatShortPrefix (testCase.encoding) + " -> float");
        }
        {
            // read into double
            wav::Reader reader {filename};
            std::vector<double> actual (reader.samples());
            const auto samplesRead = reader.read (actual);
            log.Record (samplesRead == testCase.expected.size() && areEqual (actual, testCase.expected), "Sample Decoding - " + exportFormatShortPrefix (testCase.encoding) + " -> double");
        }
    }
    std::remove (filename.c_str());
}
// Build a WAVEFORMATEX WAV file (18-byte fmt chunk).
//
// Layout:
//   RIFF header   (12 bytes)
//   fmt  chunk    (8 + 18 = 26 bytes)
//   data chunk    (8 + dataSize bytes)
//   padding byte  (if dataSize is odd)
//
inline std::vector<uint8_t> buildWAVEFORMATEX (wav::ExportFormat encoding, uint16_t channels, uint32_t sampleRate, std::vector<uint8_t> const & data)
{
    const auto [bitDepth, formatCode] = bitDepthAndFormatCode (encoding);
    constexpr uint32_t formatChunkSize {wav::FormatChunkSize::WAVEFORMATEX};

    assert (bitDepth % 8 == 0);

    const auto blockAlign {static_cast<uint16_t> (channels * bitDepth / 8)};
    const auto byteRate {sampleRate * blockAlign};
    const auto dataChunkSize {static_cast<uint32_t> (data.size())};

    const uint16_t cbSize {0};

    std::vector<uint8_t> wav;

    // RIFF
    push_riff_header (wav, formatChunkSize, dataChunkSize);

    // fmt
    push_fmt_header        (wav, formatChunkSize);
    push_WAVEFORMAT_fmt    (wav, formatCode, channels, sampleRate, byteRate, blockAlign); // 14 bytes
    push_PCMWAVEFORMAT_fmt (wav, bitDepth);                                               //  2 bytes
    push_WAVEFORMATEX_fmt  (wav, cbSize);                                                 //  2 bytes
                                                                                          // --------
                                                                                          // 18 bytes

    // data
    push_data_chunk (wav, data);

    return wav;
}

#endif

