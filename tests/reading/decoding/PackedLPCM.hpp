// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_PackedLPCM_hpp
#define wavplusplus_tests_PackedLPCM_hpp

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <format>
#include <string>
#include <vector>

void WriteData (std::vector<uint8_t> &, std::vector<uint8_t> const &);

void WriteDataAndTest (std::string const &, std::vector<uint8_t> &, std::vector<uint8_t> const &, TestLog &, std::vector<float> const &, uint16_t, uint16_t, std::string const &);

struct TestCase
{
    std::vector<uint8_t> packed;
    std::vector<float> expected;
    uint16_t validBitDepth {0};
    uint16_t containerSizeInBits {0};
};

// Valid bits of packed LPCM samples are assumed to be LSB-aligned inside their containers.
// The stored signed integer is sign-extended to the container width.
//
// (The stored value is the same when interpreted in terms of full container size as
// it would be if interpreted in a container that was truncated according to the number of
// valid bits.)
//
// Dequantization: sample_float = integer / 2^(validBits - 1).

inline void TestPackedLPCM (TestLog & log)
{
    const auto filename {"test_packed_lpcm.wav"};
    constexpr uint16_t channels {1};
    constexpr uint32_t sampleRate {44100};

    std::vector <TestCase> testCases
    {
        // 12 bits packed in 16 bits
        {
            {
                0x00, 0xF8, // -2048
                0x00, 0x00, //     0
                0xFF, 0x07, // +2047
            },
            {
                -1.f,
                0.f,
                2047.f / 2048.f
            },
            12,
            16
        },
    };

    for (const auto & testCase : testCases)
    {
        const auto & expected {testCase.expected};
        const auto frames {static_cast<uint32_t> (expected.size())};
        const auto & packed {testCase.packed};
        const auto validBitDepth {testCase.validBitDepth};
        const auto containerSizeInBits {testCase.containerSizeInBits};

        {
            constexpr auto fileFormat {"PCMWAVEFORMAT"};

            auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM,
                                             validBitDepth, containerSizeInBits,
                                             channels, sampleRate, frames);

            WriteDataAndTest (filename, bytes, packed,
                              log, expected, validBitDepth, containerSizeInBits, fileFormat);
        }
        {
            constexpr auto fileFormat {"WAVEFORMATEX"};

            auto bytes = buildWAVEFORMATEX (wav::FormatCode::LinearPCM,
                                            validBitDepth, containerSizeInBits,
                                            channels, sampleRate, frames);

            WriteDataAndTest (filename, bytes, packed,
                              log, expected, validBitDepth, containerSizeInBits, fileFormat);
        }
        {
            constexpr auto fileFormat {"WAVEFORMATEXTENSIBLE"};

            auto bytes = buildWAVEFORMATEXTENSIBLE (wav::ExtensibleFormat::LinearPCM,
                                                    validBitDepth, containerSizeInBits,
                                                    channels, sampleRate, frames,
                                                    wav::SpeakerConfiguration::Unassigned);

            WriteDataAndTest (filename, bytes, packed,
                              log, expected, validBitDepth, containerSizeInBits, fileFormat);
        }
    }
}

inline void WriteData (std::vector<uint8_t> & bytes, std::vector<uint8_t> const & data)
{
    // overwrite the data chunk sample area
    const auto dataStart {chunkOffset ("data", bytes) + 8u};
    std::copy (data.cbegin(), data.cend(), bytes.begin() + static_cast<std::ptrdiff_t> (dataStart));
}

inline void WriteDataAndTest (std::string const & filename,
                              std::vector<uint8_t> & bytes, std::vector<uint8_t> const & data,
                              TestLog & log,
                              std::vector<float> const & expected,
                              uint16_t validBitDepth, uint16_t containerSizeInBits,
                              std::string const & fileFormat)
{
    WriteData (bytes, data);
    WriteBytesToFile (filename, bytes);

    for (bool interleaved : {true, false})
    {
        wav::Reader reader {filename};

        const auto channels {reader.channels()};
        const auto samples {reader.samples()};
        const auto frames {reader.frames()};

        assert (channels == 1 && samples == frames);
        assert (reader.bitDepth() == validBitDepth &&
                reader.containerSizeInBits() == containerSizeInBits);

        std::vector<float> in (samples);

        size_t readCount {0};
        if (interleaved)
        {
            readCount = reader.read (in.data(), samples);
        }
        else
        {
            float * destination[] = {in.data()};
            readCount = reader.readFrames (destination, channels, frames);
        }

        bool result
        {
            readCount == expected.size() &&
            areEqual (in, expected)
        };
        log.Record (result, std::format ("Packed LPCM - {}|{}, {}, {}", validBitDepth, containerSizeInBits, fileFormat, interleaved ? "interleaved" : "deinterleaved"));
    }

    std::remove (filename.c_str());
}

#endif
