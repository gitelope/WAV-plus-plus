
#ifndef wavplusplus_WriteHelpers_hpp
#define wavplusplus_WriteHelpers_hpp

#include "Formats.hpp"
#include "Exceptions.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <cassert>

namespace wav
{

std::pair<uint16_t,uint16_t> bitDepthAndFormatCode (ExportFormat ef);
void WriteHeader (std::ofstream & out, uint16_t bitsPerSample, uint16_t formatCode, int sampleRate, int channels, size_t numSamples);

void WriteToFile (std::ofstream & out, std::string const & str);

// Write integer or float.
//
// By default, the number of bytes is the container size,
// but it may also be less than the container size.
template <typename T>
requires (std::integral<T> || std::floating_point<T>)
void WriteToFile (std::ofstream & out, T value, int size = sizeof(value))
{
    assert (size > 0 && size <= sizeof(value));
    out.write (reinterpret_cast<const char *> (&value), size);
    if (!out)
    {
        throw WriteError ("Failed to write to file.");
    }
}

template <FloatingPoint T>
T clamped (T value, T min, T max)
{
    return std::min (std::max (min, value), max);
}

template <typename IntType, FloatingPoint T>
void WriteLinearPCM (std::ofstream & out, std::vector<T> const & samples, uint16_t bitsPerSample)
{
    // 2^(bits-1) - 1
    const auto max {static_cast<T> ((1 << (bitsPerSample - 1)) - 1)};
    const auto min {-max};
    const int bytes {bitsPerSample / 8};
    T offset {0};
    if constexpr (std::is_same_v <IntType, uint8_t>)
    {
        offset = 128;
    }
    for (auto const & sample : samples)
    {
        WriteToFile (out, static_cast<IntType> (clamped (round (sample * max /* + dither */), min, max) + offset), bytes);
    }
}

template <FloatingPoint T>
void WriteData (std::ofstream & out, std::vector<T> const & samples, uint16_t bitsPerSample, uint16_t formatCode)
{
    if (formatCode == FormatCode::LINEAR_PCM)
    {
        switch (bitsPerSample)
        {
            case 8:
            {
                WriteLinearPCM<uint8_t> (out, samples, bitsPerSample);
                break;
            }
            case 16:
            {
                WriteLinearPCM<int16_t> (out, samples, bitsPerSample);
                break;
            }
            case 24:
            {
                // When we store our value (range [-2^23 + 1, 2^23 - 1])
                // in a 32-bit 2's-complement int, the eight most
                // significant bits are just sign extension. The lowest
                // 3 bytes perfectly represent the number as a 24-bit
                // (2's-complement) signed integer.
                //
                // positive number:
                // 24-bit:          01001001 11100111 10100001 = 4,843,425
                // 32-bit: 00000000 01001001 11100111 10100001 = 4,843,425
                //
                // negative number:
                // 24-bit:          11100011 01101000 01111011 = -1,873,797
                // 32-bit: 11111111 11100011 01101000 01111011 = -1,873,797
                //
                WriteLinearPCM<int32_t> (out, samples, bitsPerSample);
                break;
            }
            case 32:
            {
                WriteLinearPCM<int32_t> (out, samples, bitsPerSample);
                break;
            }
        }
        // If an odd number of data bytes, add a padding byte.
        assert (bitsPerSample % 8 == 0);
        if (samples.size() * bitsPerSample / 8 % 2) out << static_cast<uint8_t> (0);
    }
    else if (formatCode == FormatCode::FLOAT_PCM)
    {
        switch (bitsPerSample)
        {
            case 32:
            {
                static_assert (sizeof (float) == 4 && CHAR_BIT == 8, "float is not 32-bit.");
                for (auto const & sample : samples)
                    WriteToFile (out, static_cast<float> (sample));
                break;
            }
            case 64:
            {
                static_assert (sizeof (double) == 8 && CHAR_BIT == 8, "double is not 64-bit.");
                for (auto const & sample : samples)
                    WriteToFile (out, static_cast<double> (sample));
                break;
            }
        }
    }
    else assert (false);
}

} // namespace wav

#endif
