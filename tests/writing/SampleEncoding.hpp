
#ifndef wavplusplus_tests_SampleEncoding_Tests_hpp
#define wavplusplus_tests_SampleEncoding_Tests_hpp

#include "../../include/wav-plus-plus/Writer.hpp"

#include <fstream>
#include <vector>

void TestSampleEncoding (TestLog & log)
{
    const auto filename       {"testSampleEncoding.wav"};
    constexpr auto sampleRate {48000};
    constexpr auto channels    {1};

    const std::vector<float>  out_float
    {-1.f, -0.875f, -0.75f, -0.625f, -0.5f, -0.25f, 0.f, 0.25f, 0.5f, 0.625f, 0.75f, 0.875f, 1.f};

    const std::vector<double> out_double
    {-1., -0.875, -0.75, -0.625, -0.5, -0.25, 0., 0.25, 0.5, 0.625, 0.75, 0.875, 1.};

    const auto RunTest = [&log, &filename]
    <typename T> (wav::ExportFormat encoding,
                  std::vector<T> const & source,
                  std::vector<uint8_t> const & expected,
                  std::string const & description)
    {
        std::vector<uint8_t> actual (expected.size());

        wav::Writer writer {filename, encoding, sampleRate, channels};
        writer.write (source);
        writer.Close();

        std::ifstream in {filename, std::ios::binary};

        const auto [bits, _] = wav::bitDepthAndFormatCode (encoding);
        const auto headerSize = (bits > 16) ? 68 /*WAVEFORMATEXTENSIBLE*/ : 46 /*WAVEFORMATEX*/;
        in.seekg (headerSize);

        in.read (reinterpret_cast<char *> (actual.data()),
                static_cast<std::streamsize> (actual.size()));
        log.Record (areEqual (actual, expected), "Write Encoding - " + description);
    };

    {
        // unsigned 8-bit LPCM
        constexpr auto encoding {wav::ExportFormat::LinearPCM_8};
        // multiply by 127, round to nearest integer, add 128, clamp to [1, 255]
        const std::vector<uint8_t> expected
        {
            0x01, // 0000 0001,   1, -1
            0x11, // 0001 0001,  17, -0.875
            0x21, // 0010 0001,  33, -0.75
            0x31, // 0011 0001,  49, -0.625
            0x40, // 0100 0000,  64, -0.5
            0x60, // 0110 0000,  96, -0.25
            0x80, // 1000 0000, 128,  0
            0xA0, // 1010 0000, 160,  0.25
            0xC0, // 1100 0000, 192,  0.5
            0xCF, // 1100 1111, 207,  0.625
            0xDF, // 1101 1111, 223,  0.75
            0xEF, // 1110 1111, 239,  0.875
            0xFF, // 1111 1111, 255,  1
        };

        RunTest (encoding, out_float,  expected, "float  -> u8");
        RunTest (encoding, out_double, expected, "double -> u8");
    }

    // signed 16-bit LPCM
    {
        constexpr auto encoding {wav::ExportFormat::LinearPCM_16};
        // multiply by 32767, round to nearest integer, clamp to [-32767, 32767]
        const std::vector<uint8_t> expected
        {
            0x01, 0x80, // 0x8001, 1000 0000 0000 0001, -32767, -1
            0x01, 0x90, // 0x9001, 1001 0000 0000 0001, -28671, -0.875
            0x01, 0xA0, // 0xA001, 1010 0000 0000 0001, -24575, -0.75
            0x01, 0xB0, // 0xB001, 1011 0000 0000 0001, -20479, -0.625
            0x00, 0xC0, // 0xC000, 1100 0000 0000 0000, -16384, -0.5
            0x00, 0xE0, // 0xE000, 1110 0000 0000 0000,  -8192, -0.25
            0x00, 0x00, // 0x0000, 0000 0000 0000 0000,      0,  0
            0x00, 0x20, // 0x2000, 0010 0000 0000 0000,   8192,  0.25
            0x00, 0x40, // 0x4000, 0100 0000 0000 0000,  16384,  0.5
            0xFF, 0x4F, // 0x4FFF, 0100 1111 1111 1111,  20479,  0.625
            0xFF, 0x5F, // 0x5FFF, 0101 1111 1111 1111,  24575,  0.75
            0xFF, 0x6F, // 0x6FFF, 0110 1111 1111 1111,  28671,  0.875
            0xFF, 0x7F, // 0x7FFF, 0111 1111 1111 1111,  32767,  1
        };

        RunTest (encoding, out_float,  expected, "float  -> s16");
        RunTest (encoding, out_double, expected, "double -> s16");
    }

    // signed 24-bit LPCM
    {
        constexpr auto encoding {wav::ExportFormat::LinearPCM_24};
        // multiply by 8388607, round to nearest integer, clamp to [-8388607, 8388607]
        const std::vector<uint8_t> expected
        {
            0x01, 0x00, 0x80, // 0x800001, 1000 0000 0000 0000 0000 0001, -8388607, -1
            0x01, 0x00, 0x90, // 0x900001, 1001 0000 0000 0000 0000 0001, -7340031, -0.875
            0x01, 0x00, 0xA0, // 0xA00001, 1010 0000 0000 0000 0000 0001, -6291455, -0.75
            0x01, 0x00, 0xB0, // 0xB00001, 1011 0000 0000 0000 0000 0001, -5242879, -0.625
            0x00, 0x00, 0xC0, // 0xC00000, 1100 0000 0000 0000 0000 0000, -4194304, -0.5
            0x00, 0x00, 0xE0, // 0xE00000, 1110 0000 0000 0000 0000 0000, -2097152, -0.25
            0x00, 0x00, 0x00, // 0x000000, 0000 0000 0000 0000 0000 0000,        0,  0
            0x00, 0x00, 0x20, // 0x200000, 0010 0000 0000 0000 0000 0000,  2097152,  0.25
            0x00, 0x00, 0x40, // 0x400000, 0100 0000 0000 0000 0000 0000,  4194304,  0.5
            0xFF, 0xFF, 0x4F, // 0x4FFFFF, 0100 1111 1111 1111 1111 1111,  5242879,  0.625
            0xFF, 0xFF, 0x5F, // 0x5FFFFF, 0101 1111 1111 1111 1111 1111,  6291455,  0.75
            0xFF, 0xFF, 0x6F, // 0x6FFFFF, 0110 1111 1111 1111 1111 1111,  7340031,  0.875
            0xFF, 0xFF, 0x7F, // 0x7FFFFF, 0111 1111 1111 1111 1111 1111,  8388607,  1
        };

        RunTest (encoding, out_float,  expected, "float  -> s24");
        RunTest (encoding, out_double, expected, "double -> s24");
    }

    // signed 32-bit LPCM
    {
        constexpr auto encoding {wav::ExportFormat::LinearPCM_32};
        // multiply by 2147483647, round to nearest integer, clamp to [-2147483647, 2147483647]
        const std::vector<uint8_t> expected
        {
            0x01, 0x00, 0x00, 0x80, // 0x80000001, 1000 0000 0000 0000 0000 0000 0000 0001, -2147483647, -1
            0x01, 0x00, 0x00, 0x90, // 0x90000001, 1001 0000 0000 0000 0000 0000 0000 0001, -1879048191, -0.875
            0x01, 0x00, 0x00, 0xA0, // 0xA0000001, 1010 0000 0000 0000 0000 0000 0000 0001, -1610612735, -0.75
            0x01, 0x00, 0x00, 0xB0, // 0xB0000001, 1011 0000 0000 0000 0000 0000 0000 0001, -1342177279, -0.625
            0x00, 0x00, 0x00, 0xC0, // 0xC0000000, 1100 0000 0000 0000 0000 0000 0000 0000, -1073741824, -0.5
            0x00, 0x00, 0x00, 0xE0, // 0xE0000000, 1110 0000 0000 0000 0000 0000 0000 0000,  -536870912, -0.25
            0x00, 0x00, 0x00, 0x00, // 0x00000000, 0000 0000 0000 0000 0000 0000 0000 0000,           0,  0
            0x00, 0x00, 0x00, 0x20, // 0x20000000, 0010 0000 0000 0000 0000 0000 0000 0000,   536870912,  0.25
            0x00, 0x00, 0x00, 0x40, // 0x40000000, 0100 0000 0000 0000 0000 0000 0000 0000,  1073741824,  0.5
            0xFF, 0xFF, 0xFF, 0x4F, // 0x4FFFFFFF, 0100 1111 1111 1111 1111 1111 1111 1111,  1342177279,  0.625
            0xFF, 0xFF, 0xFF, 0x5F, // 0x5FFFFFFF, 0101 1111 1111 1111 1111 1111 1111 1111,  1610612735,  0.75
            0xFF, 0xFF, 0xFF, 0x6F, // 0x6FFFFFFF, 0110 1111 1111 1111 1111 1111 1111 1111,  1879048191,  0.875
            0xFF, 0xFF, 0xFF, 0x7F, // 0x7FFFFFFF, 0111 1111 1111 1111 1111 1111 1111 1111,  2147483647,  1
        };

        RunTest (encoding, out_float,  expected, "float  -> s32");
        RunTest (encoding, out_double, expected, "double -> s32");
    }

    // 32-bit float PCM
    {
        constexpr auto encoding {wav::ExportFormat::FloatPCM_32};

        const std::vector<uint8_t> expected
        {
            0x00, 0x00, 0x80, 0xBF, // 0xBF800000, 1 | 01111111 | 00000000000000000000000, -1
            0x00, 0x00, 0x60, 0xBF, // 0xBF600000, 1 | 01111110 | 11000000000000000000000, -0.875
            0x00, 0x00, 0x40, 0xBF, // 0xBF400000, 1 | 01111110 | 10000000000000000000000, -0.75
            0x00, 0x00, 0x20, 0xBF, // 0xBF200000, 1 | 01111110 | 01000000000000000000000, -0.625
            0x00, 0x00, 0x00, 0xBF, // 0xBF000000, 1 | 01111110 | 00000000000000000000000, -0.5
            0x00, 0x00, 0x80, 0xBE, // 0xBE800000, 1 | 01111101 | 00000000000000000000000, -0.25
            0x00, 0x00, 0x00, 0x00, // 0x00000000, 0 | 00000000 | 00000000000000000000000,  0
            0x00, 0x00, 0x80, 0x3E, // 0x3E800000, 0 | 01111101 | 00000000000000000000000,  0.25
            0x00, 0x00, 0x00, 0x3F, // 0x3F000000, 0 | 01111110 | 00000000000000000000000,  0.5
            0x00, 0x00, 0x20, 0x3F, // 0x3F200000, 0 | 01111110 | 01000000000000000000000,  0.625
            0x00, 0x00, 0x40, 0x3F, // 0x3F400000, 0 | 01111110 | 10000000000000000000000,  0.75
            0x00, 0x00, 0x60, 0x3F, // 0x3F600000, 0 | 01111110 | 11000000000000000000000,  0.875
            0x00, 0x00, 0x80, 0x3F, // 0x3F800000, 0 | 01111111 | 00000000000000000000000,  1
        };

        RunTest (encoding, out_float,  expected, "float  -> f32");
        RunTest (encoding, out_double, expected, "double -> f32");
    }

    // 64-bit float PCM
    {
        constexpr auto encoding {wav::ExportFormat::FloatPCM_64};

        const std::vector<uint8_t> expected
        {
            0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0xBF, // 0xBFF0000000000000, 1 | 01111111111 | 000...0, -1
            0x00,0x00,0x00,0x00,0x00,0x00,0xEC,0xBF, // 0xBFEC000000000000, 1 | 01111111110 | 110...0, -0.875
            0x00,0x00,0x00,0x00,0x00,0x00,0xE8,0xBF, // 0xBFE8000000000000, 1 | 01111111110 | 100...0, -0.75
            0x00,0x00,0x00,0x00,0x00,0x00,0xE4,0xBF, // 0xBFE4000000000000, 1 | 01111111110 | 010...0, -0.625
            0x00,0x00,0x00,0x00,0x00,0x00,0xE0,0xBF, // 0xBFE0000000000000, 1 | 01111111110 | 000...0, -0.5
            0x00,0x00,0x00,0x00,0x00,0x00,0xD0,0xBF, // 0xBFD0000000000000, 1 | 01111111101 | 000...0, -0.25
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 0x0000000000000000, 0 | 00000000000 | 000...0, 0
            0x00,0x00,0x00,0x00,0x00,0x00,0xD0,0x3F, // 0x3FD0000000000000, 0 | 01111111101 | 000...0, 0.25
            0x00,0x00,0x00,0x00,0x00,0x00,0xE0,0x3F, // 0x3FE0000000000000, 0 | 01111111110 | 000...0, 0.5
            0x00,0x00,0x00,0x00,0x00,0x00,0xE4,0x3F, // 0x3FE4000000000000, 0 | 01111111110 | 010...0, 0.625
            0x00,0x00,0x00,0x00,0x00,0x00,0xE8,0x3F, // 0x3FE8000000000000, 0 | 01111111110 | 100...0, 0.75
            0x00,0x00,0x00,0x00,0x00,0x00,0xEC,0x3F, // 0x3FEC000000000000, 0 | 01111111110 | 110...0, 0.875
            0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0x3F, // 0x3FF0000000000000, 0 | 01111111111 | 000...0, 1
        };

        RunTest (encoding, out_float,  expected, "float  -> f64");
        RunTest (encoding, out_double, expected, "double -> f64");
    }

    std::remove (filename);
}

#endif
