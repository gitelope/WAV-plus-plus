
#include "Write.hpp"
#include <iostream>

namespace wav
{

// Forward declarations for helper functions:
template <FloatingPoint T> void WriteData (std::ofstream &, std::vector<T> const &, uint16_t, uint16_t);
template <FloatingPoint T> T clamped (T, T, T);
void WriteToFile (std::ofstream &, std::string const &);
template <typename T> requires (std::integral<T> || std::floating_point<T>)
void WriteToFile (std::ofstream &, T value, int size = sizeof(value));
////////////////////////////////////////////

std::pair<uint16_t,uint16_t> bitDepthAndFormatCode (ExportFormat ef)
{
    switch (ef)
    {
        case ExportFormat::LINEAR_PCM_8:  return {8,  FormatCode::LINEAR_PCM};
        case ExportFormat::LINEAR_PCM_16: return {16, FormatCode::LINEAR_PCM};
        case ExportFormat::LINEAR_PCM_24: return {24, FormatCode::LINEAR_PCM};
        case ExportFormat::LINEAR_PCM_32: return {32, FormatCode::LINEAR_PCM};
        case ExportFormat::FLOAT_PCM_32:  return {32, FormatCode::FLOAT_PCM};
        case ExportFormat::FLOAT_PCM_64:  return {64, FormatCode::FLOAT_PCM};
    }
}

void WriteHeader (std::ofstream & out, uint16_t bitsPerSample, uint16_t formatCode, int sampleRate, int channels, size_t numSamples)
{
    const auto bytesPerSample {bitsPerSample / 8};
    constexpr auto formatChunkSize {FormatChunkSize::PCMWAVEFORMAT};

    // Note: data chunk size will not and should not account for a padding byte
    // when samples.size() is odd. However, that padding byte will and should be
    // reflected in RIFF chunk size.

    const auto dataChunkSize {static_cast<uint32_t> (numSamples /*samples.size()*/ * bytesPerSample)};
    const uint32_t riffChunkSize

    // RIFF chunk
    = 4 // "WAVE"

    // fmt  chunk
    + 4  // "fmt "
    + 4  // chunk size
    + formatChunkSize // in bytes

    // data chunk
    + 4 // "data"
    + 4 // chunk size
    + dataChunkSize // (in bytes) num samples x bytes per sample
    + dataChunkSize % 2; // 1 byte of padding when dataChunkSize is an odd number

    // blocks/sec * samples/block * bytes/sample = bytes/sec
    const auto bytesPerSecond {static_cast<uint32_t> (sampleRate * channels * bytesPerSample)};

    // block size in bytes = samples/block * bytes/sample
    const auto blockAlign {static_cast<uint16_t> (channels * bytesPerSample)};

    WriteToFile (out, "RIFF");
    WriteToFile (out, riffChunkSize);
    WriteToFile (out, "WAVE");

    WriteToFile (out, "fmt ");
    WriteToFile (out, formatChunkSize);
    WriteToFile (out, formatCode);
    WriteToFile (out, static_cast<uint16_t> (channels));
    WriteToFile (out, static_cast<uint32_t> (sampleRate)); // blocks (a.k.a. frames) per second
    WriteToFile (out, bytesPerSecond);
    WriteToFile (out, blockAlign);
    WriteToFile (out, bitsPerSample);

    WriteToFile (out, "data");
    WriteToFile (out, dataChunkSize);
}

template <FloatingPoint T>
void Write (std::string const & filepath,
            std::vector<T> const & samples,
            ExportFormat exportFormat,
            int sampleRate,
            int channels)
{
    if (samples.size() % channels != 0)
    {
        throw std::invalid_argument ("Number of samples should be divisible by the number of channels.");
    }
    const auto [bitsPerSample, formatCode] = bitDepthAndFormatCode (exportFormat);
    assert (bitsPerSample % 8 == 0);

    std::ofstream out {filepath, std::ofstream::binary};
    if (!out)
    {
        throw WriteError ("Failed to open file for writing: " + filepath);
    }

    WriteHeader (out, bitsPerSample, formatCode, sampleRate, channels, samples.size());
    WriteData (out, samples, bitsPerSample, formatCode);

    out.close();
    if (!out)
    {
        throw WriteError ("File did not close successfully.");
    }
}
// Explicit instantiation allows separation of declaration and definition.
template void Write (std::string const &, std::vector<float>  const &, ExportFormat, int, int);
template void Write (std::string const &, std::vector<double> const &, ExportFormat, int, int);

template <FloatingPoint T>
void Normalize (std::vector<T> & samples)
{
    T maximum {0};
    for (auto const & sample : samples)
    {
        maximum = std::max (maximum, std::abs (sample));
    }
    if (maximum == 0) return;
    for (auto & sample : samples)
    {
        sample /= maximum;
    }
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

// Write string.
void WriteToFile (std::ofstream & out, std::string const & str)
{
    out.write (str.data(), str.size());
    if (!out)
    {
        throw WriteError ("Failed to write string to file.");
    }
}
// Write integer or float.
//
// By default, the number of bytes is the container size,
// but it may also be less than the container size.
template <typename T>
requires (std::integral<T> || std::floating_point<T>)
void WriteToFile (std::ofstream & out, T value, int size)
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

} // namespace wav
