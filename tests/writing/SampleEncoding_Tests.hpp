
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

    const std::vector<float>  out_float  {-1.0, -0.5, -0.25, 0, 0.25, 0.5, 1.0};
    const std::vector<double> out_double {-1.0, -0.5, -0.25, 0, 0.25, 0.5, 1.0};

    // unsigned 8-bit LPCM
    {
        const std::vector<uint8_t> expected
        {
            0x01, // 1,   0000 0001, -1.0
            0x40, // 64,  0100 0000, -0.5
            0x60, // 96,  0110 0000, -0.25
            0x80, // 128, 1000 0000,  0
            0xA0, // 160, 1010 0000,  0.25
            0xC0, // 192, 1100 0000,  0.5
            0xFF, // 255, 1111 1111,  1.0
        };

        // float
        {
            wav::Writer writer {filename, wav::ExportFormat::LinearPCM_8, sampleRate, channels};
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
            wav::Writer writer {filename, wav::ExportFormat::LinearPCM_8, sampleRate, channels};
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
        const std::vector<uint8_t> expected
        {
            0x01, 0x80, // -32767, 1000 0000 0000 0001, 0x8001, -1.0
            0x00, 0xC0, // -16384, 1100 0000 0000 0000, 0xC000, -0.5
            0x00, 0xE0, //  -8192, 1110 0000 0000 0000, 0xE000, -0.25
            0x00, 0x00, //      0, 0000 0000 0000 0000, 0x0000,  0
            0x00, 0x20, //   8192, 0010 0000 0000 0000, 0x2000,  0.25
            0x00, 0x40, //  16384, 0100 0000 0000 0000, 0x4000,  0.5
            0xFF, 0x7F, //  32767, 0111 1111 1111 1111, 0x7FFF,  1.0
        };

        // float
        {
            wav::Writer writer {filename, wav::ExportFormat::LinearPCM_16, sampleRate, channels};
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
            wav::Writer writer {filename, wav::ExportFormat::LinearPCM_16, sampleRate, channels};
            writer.write (out_double);
            writer.Close();

            std::ifstream in {filename, std::ios::binary};
            in.seekg (headerSize);
            std::vector<uint8_t> actual (expected.size());
            in.read (reinterpret_cast<char*> (actual.data()), static_cast<std::streamsize> (actual.size()));
            log.Record (areEqual (actual, expected), "double -> signed 16-bit LPCM");
        }
    }
    {
        // signed 24-bit LPCM
    }
    {
        // signed 32-bit LPCM
    }
    {
        // 32-bit float PCM
    }
    {
        // 64-bit float PCM
    }
}

#endif
