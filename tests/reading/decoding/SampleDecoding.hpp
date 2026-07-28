// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_SampleDecoding_hpp
#define wavplusplus_tests_SampleDecoding_hpp

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>

// build WAVEFORMATEX with specific data in the data chunk
std::vector<uint8_t> buildWAVEFORMATEX (wav::ExportFormat encoding, uint16_t channels, uint32_t sampleRate, std::vector<uint8_t> const & data);

void TestSampleDecoding (TestLog & log)
{
    constexpr auto sampleRate {44100};
    constexpr auto channels {1};
    const auto filename {"testSampleDecoding.wav"};

    struct TestCase
    {
        wav::ExportFormat encoding;
        std::vector<uint8_t> data;
        std::vector<double> expected;
    };

    std::vector<TestCase> monoTestCases
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
    for (auto & testCase : monoTestCases)
    {
        WriteBytesToFile (filename, buildWAVEFORMATEX (testCase.encoding, channels, sampleRate, testCase.data));

        {
            // read into float

            // (first convert expected from double to float)
            std::vector<float> expectedFloat (testCase.expected.size());
            for (size_t i = 0; i < testCase.expected.size(); ++i)
            {
                expectedFloat [i] = static_cast<float> (testCase.expected [i]);
            }

            {
                // read interleaved
                wav::Reader reader {filename};
                std::vector<float> actual (reader.samples());
                const auto samplesRead = reader.read (actual);
                const bool result
                {
                    samplesRead == expectedFloat.size() &&
                    areEqual (actual, expectedFloat)
                };
                log.Record (result, "Sample Decoding, interleaved   - " + exportFormatShortPrefix (testCase.encoding) + " -> float");
            }
            {
                // read deinterleaved
                wav::Reader reader {filename};
                std::vector<float> actual (reader.frames());
                std::vector<float *> destinations {actual.data()};
                const auto framesRead = reader.readFrames (destinations.data(), reader.channels(), reader.frames());
                const bool result
                {
                    framesRead == expectedFloat.size() &&
                    areEqual (actual, expectedFloat)
                };
                log.Record (result, "Sample Decoding, deinterleaved - " + exportFormatShortPrefix (testCase.encoding) + " -> float");
            }
        }

        {
            // read into double

            {
                // read interleaved
                wav::Reader reader {filename};
                std::vector<double> actual (reader.samples());
                const auto samplesRead = reader.read (actual);
                const bool result
                {
                    samplesRead == testCase.expected.size() &&
                    areEqual (actual, testCase.expected)
                };
                log.Record (result, "Sample Decoding, interleaved   - " + exportFormatShortPrefix (testCase.encoding) + " -> double");
            }
            {
                // read deinterleaved
                wav::Reader reader {filename};
                std::vector<double> actual (reader.frames());
                std::vector<double *> destinations {actual.data()};
                const auto framesRead = reader.readFrames (destinations.data(), reader.channels(), reader.frames());
                const bool result
                {
                    framesRead == testCase.expected.size() &&
                    areEqual (actual, testCase.expected)
                };
                log.Record (result, "Sample Decoding, deinterleaved - " + exportFormatShortPrefix (testCase.encoding) + " -> double");
            }

        }
    }
    std::remove (filename);
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
    PushRiffHeader (wav, formatChunkSize, dataChunkSize);

    // fmt
    PushFmtHeader        (wav, formatChunkSize);
    Push_WAVEFORMAT_Fmt    (wav, formatCode, channels, sampleRate, byteRate, blockAlign); // 14 bytes
    Push_PCMWAVEFORMAT_Fmt (wav, bitDepth);                                               //  2 bytes
    Push_WAVEFORMATEX_Fmt  (wav, cbSize);                                                 //  2 bytes
                                                                                          // --------
                                                                                          // 18 bytes

    // data
    PushDataChunk (wav, data);

    return wav;
}

#endif

