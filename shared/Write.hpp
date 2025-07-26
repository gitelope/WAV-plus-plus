
#ifndef wavplusplus_Write_hpp
#define wavplusplus_Write_hpp

#include "WriteHelpers.hpp"

/*

 sample encoding options:
 linear PCM: 8-bit, 16-bit, 24-bit, and 32-bit
 float PCM: 32-bit and 64-bit

 channel options:
 mono, stereo, and multichannel

 format:
 PCMWAVEFORMAT (very common, but technically non-standard for some of the above options)

 ---

 quantization (linear PCM only):

 double -> signed int (16, 24 and 32 bits):
 1. Multiply by 2^(bits-1) - 1
    (e.g. 32,767 for 16-bit linear PCM)
 2. Round to nearest integer.*
 3. Clamp to [1 - 2^(bits-1), 2^(bits-1) - 1]
    (e.g. [-32,767, 32,767] for 16-bit linear PCM)

double -> unsigned 8-bit int:
 1. Multiply by 127
 2. Add 128.
 3. Round to nearest integer.*
 4. Clamp to [1, 255].

 * Optional TPDF dither (not implemented) would come before rounding.

 */

namespace wav
{

// May throw wav::WriteError.
template <FloatOrDouble T>
void Write (std::string const & filepath,
                    std::vector<T> const & samples,
                    ExportFormat exportFormat,
                    int sampleRate,
                    int channels,
                    size_t requestedBufferSize = defaultRequestedReadBufferSize)
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
    WriteData (out, samples, bitsPerSample, formatCode, requestedBufferSize);

    out.close();
    if (!out)
    {
        throw WriteError ("File did not close successfully.");
    }
}

template <FloatOrDouble T>
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

} // namespace wav

#endif
