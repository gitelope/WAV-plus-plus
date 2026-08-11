// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_FullContainer_hpp
#define wavplusplus_tests_FullContainer_hpp

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>

// Tests reading all the supported formats both interleaved and deinterleaved.
// Covers valid bit depth equals container size.
// Packed LPCM tests are in a separate header.

// build WAVEFORMATEX with specific data in the data chunk
std::vector<uint8_t> buildWAVEFORMATEX (wav::ExportFormat encoding, uint16_t channels, uint32_t sampleRate, std::vector<uint8_t> const & data);

void TestInterleavedAndMonoDeinterleaved (TestLog & log)
{
    constexpr auto sampleRate {48000};
    constexpr auto channels {1};
    const auto filename {"testSampleDecoding.wav"};

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
                    samplesRead == reader.samples() &&
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
                    framesRead == reader.frames() &&
                    areEqual (actual, expectedFloat)
                };
                log.Record (result, "Sample Decoding, deinterleaved mono - " + exportFormatShortPrefix (testCase.encoding) + " -> float");
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
                    samplesRead == reader.samples() &&
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
                    framesRead == reader.frames() &&
                    areEqual (actual, testCase.expected)
                };
                log.Record (result, "Sample Decoding, deinterleaved mono - " + exportFormatShortPrefix (testCase.encoding) + " -> double");
            }

        }
    }
    std::remove (filename);
}

void TestDeinterleavedStereo (TestLog & log)
{
    constexpr auto sampleRate {44100};
    constexpr auto channels {2};
    const auto filename {"testSampleDecoding.wav"};

    struct TestCase
    {
        wav::ExportFormat encoding;
        std::vector<uint8_t> data;
        std::vector<double> expectedLeft;
        std::vector<double> expectedRight;
    };

    std::vector<TestCase> stereoTestCases
    {
        {
            wav::ExportFormat::LinearPCM_8,
            {
                0x00,  0xFF, //   0,  255
                0x7F,  0x81, // 127,  129
                0x80,  0x80, // 128,  128
                0x81,  0x7F, // 129,  127
                0xFF,  0x00, // 255,    0
            },
            {-1.0, -1/128., 0., 1/128., 127/128.},
            {127/128., 1/128., 0., -1/128., -1.0}
        },
        {
            wav::ExportFormat::LinearPCM_16,
            {
                0x00, 0x80,  0xFF, 0x7F, // -32768,  +32767
                0xFF, 0xFF,  0x01, 0x00, //     -1,      +1
                0x00, 0x00,  0x00, 0x00, //      0,       0
                0x01, 0x00,  0xFF, 0xFF, //     +1,      -1
                0xFF, 0x7F,  0x00, 0x80, // +32767,  -32768
            },
            {-1., -1/32768., 0., 1/32768., 32767/32768.},
            {32767/32768., 1/32768., 0., -1/32768., -1.}
        },
        {
            wav::ExportFormat::LinearPCM_24,
            {
                0x00, 0x00, 0x80,  0xFF, 0xFF, 0x7F, // -8388608,  +8388607
                0xFF, 0xFF, 0xFF,  0x01, 0x00, 0x00, //       -1,        +1
                0x00, 0x00, 0x00,  0x00, 0x00, 0x00, //        0,         0
                0x01, 0x00, 0x00,  0xFF, 0xFF, 0xFF, //       +1,        -1
                0xFF, 0xFF, 0x7F,  0x00, 0x00, 0x80, // +8388607,  -8388608
            },
            {-1., -1/8388608., 0., 1/8388608., 8388607/8388608.},
            {8388607/8388608., 1/8388608., 0., -1/8388608., -1.}
        },
        {
            wav::ExportFormat::LinearPCM_32,
            {
                0x00, 0x00, 0x00, 0x80,  0xFF, 0xFF, 0xFF, 0x7F, // -2147483648,  +2147483647
                0xFF, 0xFF, 0xFF, 0xFF,  0x01, 0x00, 0x00, 0x00, //          -1            +1
                0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, //           0             0
                0x01, 0x00, 0x00, 0x00,  0xFF, 0xFF, 0xFF, 0xFF, //          +1            -1
                0xFF, 0xFF, 0xFF, 0x7F,  0x00, 0x00, 0x00, 0x80, // +2147483647   -2147483648
            },
            {-1., -1/2147483648., 0., 1/2147483648., 2147483647/2147483648.},
            {2147483647/2147483648., 1/2147483648., 0., -1/2147483648., -1.}
        },
        {
            wav::ExportFormat::FloatPCM_32,
            {
                0x00, 0x00, 0x80, 0xBF,  0x00, 0x00, 0x80, 0x3F, // -1,    +1
                0x00, 0x00, 0x00, 0xBF,  0x00, 0x00, 0x00, 0x3F, // -0.5,  +0.5
                0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, //  0,     0
                0x00, 0x00, 0x00, 0x3F,  0x00, 0x00, 0x00, 0xBF, // +0.5,  -0.5
                0x00, 0x00, 0x80, 0x3F,  0x00, 0x00, 0x80, 0xBF, // +1,    -1
            },
            {-1., -0.5, 0., 0.5, 1.},
            {1., 0.5, 0., -0.5, -1.}
        },
        {
            wav::ExportFormat::FloatPCM_64,
            {
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xBF,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, // -1,    +1
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xBF,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x3F, // -0.5,  +0.5
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //  0,     0
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x3F,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xBF, // +0.5,  -0.5
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xBF, // +1,    -1
            },
            {-1., -0.5, 0., 0.5, 1.},
            {1., 0.5, 0., -0.5, -1.}
        },

    };

    constexpr double sentinelValue {42};

    for (auto & testCase : stereoTestCases)
    {
        WriteBytesToFile (filename, buildWAVEFORMATEX (testCase.encoding, channels, sampleRate, testCase.data));

        // create float version of expected input
        const auto expectedSize {testCase.expectedLeft.size()};
        assert (testCase.expectedLeft.size() == testCase.expectedRight.size());
        std::vector<float> expectedFloatLeft  (expectedSize);
        std::vector<float> expectedFloatRight (expectedSize);
        for (size_t i = 0; i < expectedSize; ++i)
        {
            expectedFloatLeft  [i] = static_cast<float> (testCase.expectedLeft  [i]);
            expectedFloatRight [i] = static_cast<float> (testCase.expectedRight [i]);
        }

        {
            // read deinterleaved into float

            wav::Reader reader {filename};
            std::vector<float> actualLeft  (reader.frames());
            std::vector<float> actualRight (reader.frames());
            std::vector<float *> destinations {actualLeft.data(), actualRight.data()};
            const auto framesRead = reader.readFrames (destinations.data(), reader.channels(), reader.frames());
            const bool result
            {
                framesRead == reader.frames() &&
                areEqual (actualLeft,  expectedFloatLeft) &&
                areEqual (actualRight, expectedFloatRight)
            };
            log.Record (result, "Sample Decoding, deinterleaved stereo - " + exportFormatShortPrefix (testCase.encoding) + " -> float");
        }

        {
            // read deinterleaved into double

            wav::Reader reader {filename};
            std::vector<double> actualLeft  (reader.frames());
            std::vector<double> actualRight (reader.frames());
            std::vector<double *> destinations {actualLeft.data(), actualRight.data()};
            const auto framesRead = reader.readFrames (destinations.data(), reader.channels(), reader.frames());
            const bool result
            {
                framesRead == reader.frames() &&
                areEqual (actualLeft,  testCase.expectedLeft) &&
                areEqual (actualRight, testCase.expectedRight)
            };
            log.Record (result, "Sample Decoding, deinterleaved stereo - " + exportFormatShortPrefix (testCase.encoding) + " -> double");

        }

        constexpr size_t channelsRequested {1};

        {
            // read deinterleaved into float, first channel only

            wav::Reader reader {filename};

            // 1. request all frames, get all frames.
            std::vector<float> actualLeft (reader.frames());
            float * destination[] {actualLeft.data()};
            const auto framesRead1 = reader.readFrames (destination, channelsRequested, reader.frames());

            // 2. request all frames again, get none.
            const std::vector<float> sentinelValues (reader.frames(), sentinelValue);
            std::vector<float> secondAttempt (sentinelValues);
            float * destination2[] {secondAttempt.data()};
            const auto framesRead2 = reader.readFrames (destination2, channelsRequested, reader.frames());

            const bool result
            {
                framesRead1 == reader.frames() &&
                framesRead2 == 0 &&
                areEqual (actualLeft, expectedFloatLeft) &&
                areEqual (secondAttempt, sentinelValues)
            };

            log.Record (result, "Sample Decoding, deinterleaved, requesting 1 of 2 channels - " + exportFormatShortPrefix (testCase.encoding) + " -> float");
        }

        {
            // read deinterleaved into double, first channel only

            wav::Reader reader {filename};

            // 1. request all frames, get all frames.
            std::vector<double> actualLeft (reader.frames());
            double * destination[] {actualLeft.data()};
            const auto framesRead1 = reader.readFrames (destination, channelsRequested, reader.frames());

            // 2. request all frames again, get none.
            const std::vector<double> sentinelValues (reader.frames(), sentinelValue);
            std::vector<double> secondAttempt (sentinelValues);
            double * destination2[] {secondAttempt.data()};
            const auto framesRead2 = reader.readFrames (destination2, channelsRequested, reader.frames());

            const bool result
            {
                framesRead1 == reader.frames() &&
                framesRead2 == 0 &&
                areEqual (actualLeft, testCase.expectedLeft) &&
                areEqual (secondAttempt, sentinelValues)
            };
            log.Record (result, "Sample Decoding, deinterleaved, requesting 1 of 2 channels - " + exportFormatShortPrefix (testCase.encoding) + " -> double");
        }
    }
    std::remove (filename);
}

void TestDeinterleavedThreeChannelSubset (TestLog & log)
{
    constexpr auto sampleRate {48000};
    constexpr auto channels {3};
    const auto filename {"testSampleDecodingThreeChannel.wav"};

    // 3 channels, 3 frames, 32-bit float
    const std::vector<uint8_t> data
    {
        0x00, 0x00, 0x80, 0xBF,  0x00, 0x00, 0x40, 0xBF,  0x00, 0x00, 0x00, 0xBF, // frame 0: ch0, ch1, ch2  (-1,   -0.75, -0.5 )
        0x00, 0x00, 0x80, 0xBE,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x80, 0x3E, // frame 1: ch0, ch1, ch2  (-0.25, 0,     0.25)
        0x00, 0x00, 0x00, 0x3F,  0x00, 0x00, 0x40, 0x3F,  0x00, 0x00, 0x80, 0x3F, // frame 2: ch0, ch1, ch2  ( 0.5,  0.75,  1   )
    };
    const std::vector<float> expectedCh0 {-1.f, -0.25f, 0.5f};
    const std::vector<float> expectedCh1 {-0.75f, 0.f, 0.75f};

    WriteBytesToFile (filename, buildWAVEFORMATEX (wav::ExportFormat::FloatPCM_32, channels, sampleRate, data));

    constexpr float sentinelValue {42};

    {
        // request 1 of 3 channels

        constexpr size_t channelsRequested {1};

        wav::Reader reader {filename};
        assert (reader.frames() == 3);
        assert (reader.channels() == channels);

        // 1. request all the frames, get all the frames.
        std::vector<float> ch0 (reader.frames());
        float * destination[] {ch0.data()};
        const auto framesRead1 = reader.readFrames (destination, channelsRequested, reader.frames());

        // 2. request all the frames again, get 0.
        const std::vector<float> allSentinels (reader.frames(), sentinelValue);
        std::vector<float> secondAttempt (allSentinels);
        float * destination2[] {secondAttempt.data()};
        const auto framesRead2 = reader.readFrames (destination2, channelsRequested, reader.frames());

        const bool result
        {
            framesRead1 == reader.frames() &&
            framesRead2 == 0 &&
            areEqual (ch0, expectedCh0) &&
            areEqual (secondAttempt, allSentinels)
        };
        log.Record (result, "Sample Decoding, deinterleaved, requesting 1 of 3 channels");
    }

    {
        // request 2 of 3 channels

        wav::Reader reader {filename};

        constexpr size_t channelsRequested {2};

        // 1. request all the frames, get all the frames.
        std::vector<float> ch0 (reader.frames());
        std::vector<float> ch1 (reader.frames());
        float * destinations[] {ch0.data(), ch1.data()};
        const auto framesRead1 = reader.readFrames (destinations, channelsRequested, reader.frames());

        // 2. request all the frames again, get 0.
        const std::vector<float> allSentinels (reader.frames(), sentinelValue);
        std::vector<float> secondAttemptCh0 (allSentinels);
        std::vector<float> secondAttemptCh1 (allSentinels);
        float * destinations2[] {secondAttemptCh0.data(), secondAttemptCh1.data()};
        const auto framesRead2 = reader.readFrames (destinations2, channelsRequested, reader.frames());

        const bool result
        {
            framesRead1 == reader.frames() &&
            framesRead2 == 0 &&
            areEqual (ch0, expectedCh0) &&
            areEqual (ch1, expectedCh1) &&
            areEqual (secondAttemptCh0, allSentinels) &&
            areEqual (secondAttemptCh1, allSentinels)
        };
        log.Record (result, "Sample Decoding, deinterleaved, requesting 2 of 3 channels");
    }

    std::remove (filename);
}

inline void TestFullContainer (TestLog & log)
{
    TestInterleavedAndMonoDeinterleaved (log);
    TestDeinterleavedStereo (log); // both channels and just first channel
    TestDeinterleavedThreeChannelSubset (log);
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

