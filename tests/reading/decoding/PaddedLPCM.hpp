// WAV++ (c) 2025 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_PaddedLPCM_hpp
#define wavplusplus_tests_PaddedLPCM_hpp

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
    std::vector<uint8_t> padded;
    std::vector<float> expected;
    uint16_t validBitDepth {0};
    uint16_t containerSizeInBits {0};
};

// The meaningful bits of padded LPCM samples are MSB-aligned
// inside their containers. The extra lower bits are zero padding.

inline void TestPaddedLPCM (TestLog & log)
{
    const auto filename {"test_padded_lpcm.wav"};
    constexpr uint16_t channels {1};
    constexpr uint32_t sampleRate {44100};

    std::vector <TestCase> testCases
    {
        // 6 in 8 (lowest two bits are 00)
        {
            {
                0x00, //  0
                0x80, // 32, 1000 0000 (32 padded with 00 looks like 128)
                0xFC, // 63, 1111 1100 (63 padded with 00 looks like 252)
            },
            {
                -1.f,       // = (0   - 128) / 128 = (0  - 32) / 32
                0.f,        // = (128 - 128) / 128 = (32 - 32) / 32
                31.f / 32.f // = (252 - 128) / 128 = (63 - 32) / 32
            },
            6,
            8
        },
        // 12 in 16 (lowest nibble is 0000)
        {
            {
                0x00, 0x80, // -2048 padded with 0000 looks like -32,768
                0x00, 0x00, //     0
                0xF0, 0x7F, // +2047 padded with 0000 looks like +32,752
            },
            {
                -1.f,           // = -32768 / 32768 = -2048 / 2048
                0.f,
                2047.f / 2048.f // = +32752 / 32768 = +2047 / 2048
            },
            12,
            16
        },
        // 20 in 24 (lowest nibble is 0000)
        {
            {
                0x00, 0x00, 0x80, // -524,288 padded with 0000 looks like -8,388,608
                0x00, 0x00, 0x00, //        0
                0xF0, 0xFF, 0x7F, // +524,287 padded with 0000 looks like +8,388,592
            },
            {
                -1.f,               // -8,388,608 / 8,388,608 = -524,288 / 524,288
                0.f,
                524287.f / 524288.f // +8,388,592 / 8,388,608 = +524,287 / 524,288
            },
            20,
            24
        },
        // 24 in 32 (lowest byte is 0x00)
        {
            {
                0x00, 0x00, 0x00, 0x80, // -8,388,608 padded with 0x00 looks like -2,147,483,648
                0x00, 0x00, 0x00, 0x00, //          0
                0x00, 0xFF, 0xFF, 0x7F, // +8,388,607 padded with 0x00 looks like +2,147,483,392
            },
            {
                -1.f,                 // -2,147,483,648 / 2,147,483,648 = -8,388,608 / 8,388,608
                0.f,
                8388607.f / 8388608.f // +2,147,483,392 / 2,147,483,648 = +8,388,607 / 8,388,608
            },
            24,
            32
        },
    };

    for (const auto & testCase : testCases)
    {
        const auto & expected {testCase.expected};
        const auto frames {static_cast<uint32_t> (expected.size())};
        const auto & padded {testCase.padded};
        const auto validBitDepth {testCase.validBitDepth};
        const auto containerSizeInBits {testCase.containerSizeInBits};

        constexpr auto fileFormat {"WAVEFORMATEXTENSIBLE"};

        auto bytes = buildWAVEFORMATEXTENSIBLE (wav::ExtensibleFormat::LinearPCM,
                                                validBitDepth, containerSizeInBits,
                                                channels, sampleRate, frames,
                                                wav::SpeakerConfiguration::Unassigned);

        WriteDataAndTest (filename, bytes, padded,
                          log, expected, validBitDepth, containerSizeInBits, fileFormat);
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
        log.Record (result, std::format ("Padded LPCM - {}|{}, {}, {}", validBitDepth, containerSizeInBits, fileFormat, interleaved ? "interleaved" : "deinterleaved"));
    }

    std::remove (filename.c_str());
}

#endif
