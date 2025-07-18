
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

template <FloatingPoint T>
void Write (std::string const & filepath,
            std::vector<T> const & samples,
            ExportFormat format,
            int sampleRate,
            int channels)
{
    if (samples.size() % channels != 0) throw std::invalid_argument ("Number of samples should be divisible by the number of channels.");

    uint16_t bitsPerSample {0};
    uint16_t formatCode {0};
    switch (format)
    {
        case ExportFormat::LINEAR_PCM_8:  bitsPerSample =  8; formatCode = FormatCode::LINEAR_PCM; break;
        case ExportFormat::LINEAR_PCM_16: bitsPerSample = 16; formatCode = FormatCode::LINEAR_PCM; break;
        case ExportFormat::LINEAR_PCM_24: bitsPerSample = 24; formatCode = FormatCode::LINEAR_PCM; break;
        case ExportFormat::LINEAR_PCM_32: bitsPerSample = 32; formatCode = FormatCode::LINEAR_PCM; break;
        case ExportFormat::FLOAT_PCM_32:  bitsPerSample = 32; formatCode = FormatCode::FLOAT_PCM;  break;
        case ExportFormat::FLOAT_PCM_64:  bitsPerSample = 64; formatCode = FormatCode::FLOAT_PCM;  break;
    }
    assert (bitsPerSample % 8 == 0);

    const auto bytesPerSample {bitsPerSample / 8};

    constexpr auto formatChunkSize {FormatChunkSize::PCMWAVEFORMAT};

    // Note: excludes potential padding byte when samples.size() is odd:
    const auto dataChunkSize {static_cast<uint32_t>(samples.size() * bytesPerSample)};

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
    // Note that a padding bytes are NOT reflected in the data chunk size but
    // WILL be reflected in the RIFF chunk size.

    // blocks/sec * samples/block * bytes/sample = bytes/sec
    const auto bytesPerSecond {static_cast<uint32_t> (sampleRate * channels * bytesPerSample)};

    // block size in bytes = samples/block * bytes/sample
    const auto blockAlign {static_cast<uint16_t> (channels * bytesPerSample)};

    std::ofstream out {filepath, std::ofstream::binary};
    if (!out) throw WriteError ("Failed to open file for writing: " + filepath);

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
    WriteData (out, samples, bitsPerSample, formatCode);

    out.close();
    if (!out) throw WriteError ("File did not close successfully.");
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

template <FloatingPoint T>
void WriteData (std::ofstream & out, std::vector<T> const & samples, uint16_t bitsPerSample, uint16_t formatCode)
{
    if (formatCode == FormatCode::LINEAR_PCM)
    {
        // 2^(bits-1) - 1
        const auto max {static_cast<T> ((1 << (bitsPerSample - 1)) - 1)};
        const auto min {-max};
        switch (bitsPerSample)
        {
            case 8:
            {
                const T offset {128}; // 2^(bits-1)
                for (auto const & sample : samples)
                {
                    WriteToFile (out, static_cast<uint8_t> (clamped (round (sample * max + offset /* + dither */), min+offset, max+offset)));
                }
                break;
            }
            case 16:
            {
                for (auto const & sample : samples)
                {
                    WriteToFile (out, static_cast<int16_t> (clamped (round (sample * max /* + dither */), min, max)));
                }
                break;
            }
            case 24:
            {
                for (auto const & sample : samples)
                {
                    constexpr int valid_bytes = 3;
                    // When we store our 24-bit 2's-complement int in the range [-2^23 + 1, 2^23 - 1]
                    // in a 32-bit 2's-complement int, we can ignore the 8 leftmost bits since they are just
                    // sign extension–1111 1111 if the MSB of our 24-bit int is 1 and 0000 0000 if it is 0.
                    //
                    // For example these represent the exact same value:
                    //
                    // 32-bit: 1111 1111 1010 0011 0110 1000 0111 1011 = -6,068,101
                    // 24-bit:           1010 0011 0110 1000 0111 1011 = -6,068,101
                    //
                    // 32-bit: 0000 0000 0010 1001 1110 0111 1010 0001 = 2,746,273
                    // 24-bit:           0010 1001 1110 0111 1010 0001 = 2,746,273
                    //
                    WriteToFile (out, static_cast<int32_t> (clamped (round (sample * max /* + dither */), min, max)), valid_bytes);
                }
                break;
            }
            case 32:
            {
                for (auto const & sample : samples)
                {
                    WriteToFile (out, static_cast<int32_t> (clamped (round (sample * max /* + dither */), min, max)));
                }
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
