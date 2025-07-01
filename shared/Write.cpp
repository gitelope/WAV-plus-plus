
#include "Write.hpp"
#include <fstream>
#include <cassert>

namespace wav
{

void Normalize (std::vector<double> & samples)
{
    double maximum {0};
    for (auto const & sample : samples)
    {
        maximum = std::max (maximum, std::abs (sample));
    }
    for (auto & sample : samples)
    {
        sample /= maximum;
    }
}

template <typename T>
void WriteToFile (std::ofstream & out, T value)
{
    out.write (reinterpret_cast<const char *> (&value), sizeof (value));
}

template <typename T>
void WriteToFile (std::ofstream & out, T value, int bytes)
{
    assert (bytes > 0 && bytes <= sizeof(value));
    out.write (reinterpret_cast<const char *> (&value), bytes);
}

double clamped (double value, double min, double max)
{
    return std::min (std::max (min, value), max);
}

void WriteData (std::ofstream & out, std::vector<double> const & samples, uint16_t bits_per_sample, uint16_t format_code)
{
    if (format_code == FormatCode::LINEAR_PCM)
    {
        const double max = pow (2.0, bits_per_sample - 1) - 1;
        const double min = -max;
        switch (bits_per_sample)
        {
            case 8:
            {
                const double offset = pow (2.0, bits_per_sample - 1);
                for (auto const & sample : samples)
                {
                    out << static_cast<uint8_t> (clamped (round (sample * max + offset /* + dither */), min+offset, max+offset));
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
        assert (bits_per_sample % 8 == 0);
        if (samples.size() * bits_per_sample / 8 % 2) out << static_cast<uint8_t> (0);
    }
    else if (format_code == FormatCode::FLOAT_PCM)
    {
        switch (bits_per_sample)
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
                    WriteToFile (out, sample);
                break;
            }
        }
    }
    else assert (false);
}

void Write (std::vector<double> const & samples, ExportFormat format, int sample_rate, int channels, std::string const & filepath)
{
    static_assert (std::endian::native == std::endian::little, "Only little endian systems are supported.");

    if (samples.size() % channels != 0) throw std::invalid_argument ("Number of samples should be divisible by the number of channels.");

    uint16_t bits_per_sample {0};
    uint16_t format_code {0};
    switch (format)
    {
        case ExportFormat::LINEAR_PCM_8:  bits_per_sample =  8; format_code = FormatCode::LINEAR_PCM; break;
        case ExportFormat::LINEAR_PCM_16: bits_per_sample = 16; format_code = FormatCode::LINEAR_PCM; break;
        case ExportFormat::LINEAR_PCM_24: bits_per_sample = 24; format_code = FormatCode::LINEAR_PCM; break;
        case ExportFormat::LINEAR_PCM_32: bits_per_sample = 32; format_code = FormatCode::LINEAR_PCM; break;
        case ExportFormat::FLOAT_PCM_32:  bits_per_sample = 32; format_code = FormatCode::FLOAT_PCM;  break;
        case ExportFormat::FLOAT_PCM_64:  bits_per_sample = 64; format_code = FormatCode::FLOAT_PCM;  break;
    }
    assert (bits_per_sample % 8 == 0);
    const int bytes_per_sample = bits_per_sample / 8;

    const int num_samples = static_cast<int> (samples.size());

    constexpr uint32_t format_chunk_size = 16; // PCMWAVEFORMAT size
    const uint32_t data_chunk_size = num_samples * bytes_per_sample; // excludes potential padding byte when num_samples is odd

    const uint32_t RIFF_chunk_size
    // RIFF chunk
    = 4 // "WAVE"

    // fmt  chunk
    + 4  // "fmt "
    + 4  // chunk size
    + format_chunk_size // in bytes

    // data chunk
    + 4 // "data"
    + 4 // chunk size
    + data_chunk_size // (in bytes) num samples x bytes per sample
    + data_chunk_size % 2; // equals 1 byte of padding when data_chunk_size is an odd number.
    // e.g. mono 24-bit, 7 samples -> 1 sample per block x 7 blocks x 3 bytes per sample = 21 bytes
    // Note that a padding byte is NOT reflected in the data chunk size but WILL be reflected in the RIFF chunk size.

    // blocks/sec * samples/block * bytes/sample = bytes/sec
    const uint32_t bytes_per_second = sample_rate * channels * bytes_per_sample;

    // block size in bytes = samples/block * bytes/sample
    const uint16_t block_align = channels * bytes_per_sample;

    std::ofstream out {filepath, std::ofstream::binary};

    out << "RIFF";
    WriteToFile (out, RIFF_chunk_size);
    out << "WAVE";

    out << "fmt ";
    WriteToFile (out, format_chunk_size);
    WriteToFile (out, format_code);
    WriteToFile (out, static_cast<uint16_t> (channels));
    WriteToFile (out, static_cast<uint32_t> (sample_rate)); // blocks (a.k.a. frames) per second
    WriteToFile (out, bytes_per_second);
    WriteToFile (out, block_align);
    WriteToFile (out, bits_per_sample);

    out << "data";
    WriteToFile (out, data_chunk_size);
    WriteData (out, samples, bits_per_sample, format_code);

    out.close();
}

} // namespace wav
