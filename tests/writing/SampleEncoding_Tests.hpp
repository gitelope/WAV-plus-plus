
#ifndef wavplusplus_tests_SampleEncoding_Tests_hpp
#define wavplusplus_tests_SampleEncoding_Tests_hpp

#include "../helper_functions.hpp"
#include "../../include/wav-plus-plus/Writer.hpp"

#include <fstream>
#include <vector>

void TestSampleEncoding (TestLog & log)
{
    const std::string filename {"test.wav"};
    constexpr auto sampleRate {48000};
    constexpr auto channels {1};
    constexpr auto headerSize {44}; // this could change.

    const std::vector<float>  out_float  {-1.f, -0.5f, -0.25f, 0.f, 0.25f, 0.5f, 1.f};
    const std::vector<double> out_double {-1.,  -0.5,  -0.25,  0.,  0.25,  0.5,  1. };

    // unsigned 8-bit LPCM
    {
        constexpr auto encoding {wav::ExportFormat::LinearPCM_8};
        const std::vector<uint8_t> expected
        {
            0x01, // 0000 0001,   1, -1
            0x40, // 0100 0000,  64, -0.5
            0x60, // 0110 0000,  96, -0.25
            0x80, // 1000 0000, 128,  0
            0xA0, // 1010 0000, 160,  0.25
            0xC0, // 1100 0000, 192,  0.5
            0xFF, // 1111 1111, 255,  1
        };

        // float
        {
            wav::Writer writer {filename, encoding, sampleRate, channels};
            writer.write (out_float);
            writer.Close();

            std::ifstream in {filename, std::ios::binary};
            in.seekg (headerSize);
            std::vector<uint8_t> actual (expected.size());
            in.read (reinterpret_cast<char*> (actual.data()), static_cast<std::streamsize> (actual.size()));
            log.Record (areEqual (actual, expected), "float -> unsigned 8-bit LPCM");
        }

        // double
        {
            wav::Writer writer {filename, encoding, sampleRate, channels};
            writer.write (out_double);
            writer.Close();

            std::ifstream in {filename, std::ios::binary};
            in.seekg (headerSize);
            std::vector<uint8_t> actual (expected.size());
            in.read (reinterpret_cast<char*> (actual.data()), static_cast<std::streamsize> (actual.size()));
            log.Record (areEqual (actual, expected), "double -> unsigned 8-bit LPCM");
        }
    }

    // signed 16-bit LPCM
    {
        constexpr auto encoding {wav::ExportFormat::LinearPCM_16};
        const std::vector<uint8_t> expected
        {
            0x01, 0x80, // 0x8001, 1000 0000 0000 0001, -32767, -1
            0x00, 0xC0, // 0xC000, 1100 0000 0000 0000, -16384, -0.5
            0x00, 0xE0, // 0xE000, 1110 0000 0000 0000,  -8192, -0.25
            0x00, 0x00, // 0x0000, 0000 0000 0000 0000,      0,  0
            0x00, 0x20, // 0x2000, 0010 0000 0000 0000,   8192,  0.25
            0x00, 0x40, // 0x4000, 0100 0000 0000 0000,  16384,  0.5
            0xFF, 0x7F, // 0x7FFF, 0111 1111 1111 1111,  32767,  1
        };

        // float
        {
            wav::Writer writer {filename, encoding, sampleRate, channels};
            writer.write (out_float);
            writer.Close();

            std::ifstream in {filename, std::ios::binary};
            in.seekg (headerSize);
            std::vector<uint8_t> actual (expected.size());
            in.read (reinterpret_cast<char*> (actual.data()), static_cast<std::streamsize> (actual.size()));
            log.Record (areEqual (actual, expected), "float -> signed 16-bit LPCM");
        }

        // double
        {
            wav::Writer writer {filename, encoding, sampleRate, channels};
            writer.write (out_double);
            writer.Close();

            std::ifstream in {filename, std::ios::binary};
            in.seekg (headerSize);
            std::vector<uint8_t> actual (expected.size());
            in.read (reinterpret_cast<char*> (actual.data()), static_cast<std::streamsize> (actual.size()));
            log.Record (areEqual (actual, expected), "double -> signed 16-bit LPCM");
        }
    }

    // signed 24-bit LPCM
    {
        constexpr auto encoding {wav::ExportFormat::LinearPCM_24};
        const std::vector<uint8_t> expected
        {
            0x01, 0x00, 0x80, // 0x800001, 1000 0000 0000 0000 0000 0001, -8388607, -1
            0x00, 0x00, 0xC0, // 0xC00000, 1100 0000 0000 0000 0000 0000, -4194304, -0.5
            0x00, 0x00, 0xE0, // 0xE00000, 1110 0000 0000 0000 0000 0000, -2097152, -0.25
            0x00, 0x00, 0x00, // 0x000000, 0000 0000 0000 0000 0000 0000,        0,  0
            0x00, 0x00, 0x20, // 0x200000, 0010 0000 0000 0000 0000 0000,  2097152,  0.25
            0x00, 0x00, 0x40, // 0x400000, 0100 0000 0000 0000 0000 0000,  4194304,  0.5
            0xFF, 0xFF, 0x7F, // 0x7FFFFF, 0111 1111 1111 1111 1111 1111,  8388607,  1
        };

        // float
        {
            wav::Writer writer {filename, encoding, sampleRate, channels};
            writer.write (out_float);
            writer.Close();

            std::ifstream in {filename, std::ios::binary};
            in.seekg (headerSize);
            std::vector<uint8_t> actual (expected.size());
            in.read (reinterpret_cast<char*> (actual.data()), static_cast<std::streamsize> (actual.size()));
            log.Record (areEqual (actual, expected), "float -> signed 24-bit LPCM");
            
        }

        // double
        {
            wav::Writer writer {filename, encoding, sampleRate, channels};
            writer.write (out_double);
            writer.Close();

            std::ifstream in {filename, std::ios::binary};
            in.seekg (headerSize);
            std::vector<uint8_t> actual (expected.size());
            in.read (reinterpret_cast<char*> (actual.data()), static_cast<std::streamsize> (actual.size()));
            log.Record (areEqual (actual, expected), "double -> signed 24-bit LPCM");
        }

    }

    // signed 32-bit LPCM
    {
        constexpr auto encoding {wav::ExportFormat::LinearPCM_32};
        const std::vector<uint8_t> expected
        {
            0x01, 0x00, 0x00, 0x80, // 0x80000001, 1000 0000 0000 0000 0000 0000 0000 0001, -2,147,483,647, -1
            0x00, 0x00, 0x00, 0xC0, // 0xC0000000, 1100 0000 0000 0000 0000 0000 0000 0000, -1,073,741,824, -0.5
            0x00, 0x00, 0x00, 0xE0, // 0xE0000000, 1110 0000 0000 0000 0000 0000 0000 0000,   -536,870,912, -0.25
            0x00, 0x00, 0x00, 0x00, // 0x00000000, 0000 0000 0000 0000 0000 0000 0000 0000,              0,  0
            0x00, 0x00, 0x00, 0x20, // 0x20000000, 0010 0000 0000 0000 0000 0000 0000 0000,    536,870,912,  0.25
            0x00, 0x00, 0x00, 0x40, // 0x40000000, 0100 0000 0000 0000 0000 0000 0000 0000,  1,073,741,824,  0.5
            0xFF, 0xFF, 0xFF, 0x7F, // 0x7FFFFFFF, 0111 1111 1111 1111 1111 1111 1111 1111,  2,147,483,647,  1
        };

        // float
        {
            wav::Writer writer {filename, encoding, sampleRate, channels};
            writer.write (out_float);
            writer.Close();

            std::ifstream in {filename, std::ios::binary};
            in.seekg (headerSize);
            std::vector<uint8_t> actual (expected.size());
            in.read (reinterpret_cast<char*> (actual.data()), static_cast<std::streamsize> (actual.size()));
            log.Record (areEqual (actual, expected), "float -> signed 32-bit LPCM");
        }

        // double
        {
            wav::Writer writer {filename, encoding, sampleRate, channels};
            writer.write (out_double);
            writer.Close();

            std::ifstream in {filename, std::ios::binary};
            in.seekg (headerSize);
            std::vector<uint8_t> actual (expected.size());
            in.read (reinterpret_cast<char*> (actual.data()), static_cast<std::streamsize> (actual.size()));
            log.Record (areEqual (actual, expected), "double -> signed 32-bit LPCM");
        }
    }

    // 32-bit float PCM
    {
        constexpr auto encoding {wav::ExportFormat::FloatPCM_32};
    }

    // 64-bit float PCM
    {
        constexpr auto encoding {wav::ExportFormat::FloatPCM_64};
    }
}

#endif
