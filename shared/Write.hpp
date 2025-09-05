
#ifndef wavplusplus_Write_hpp
#define wavplusplus_Write_hpp

#include "WriteHelpers.hpp"
#include <cstdio>

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

// 32kB default (requested) buffer size
inline constexpr size_t defaultRequestedWriteBufferSize {32768};

// Internal I/O buffer size will be this multiple of the actual–not requested–write buffer size.
inline constexpr size_t defaultIOBufferSizeMultiplier {8};

// May throw wav::WriteError.
template <FloatOrDouble T>
void Write (std::string const & filepath,
            std::vector<T> const & samples,
            ExportFormat exportFormat,
            int sampleRate,
            int channels,
            // requested write buffer size used by WAV++
            size_t requestedWriteBufferSize = defaultRequestedWriteBufferSize,
            // internal IO buffer size used by OS is a multiple of actual–not requested–write buffer size
            size_t ioBufferSizeMultiplier = defaultIOBufferSizeMultiplier)
{
    if (samples.size() % channels != 0)
    {
        throw std::invalid_argument ("Number of samples should be divisible by the number of channels.");
    }
    const auto [bitsPerSample, formatCode] = bitDepthAndFormatCode (exportFormat);
    assert (bitsPerSample % 8 == 0);

    // Open FILE and ensure that attempt is made to close file if WriteHeader() or WriteData() throw.
    // (fclose failure during stack unwinding is ignored to avoid throwing another exception and thus a call to std::terminate.)
    std::unique_ptr<FILE, decltype (&fclose)> out (fopen (filepath.c_str(), "wb"), &fclose);

    if (!out)
    {
        throw WriteError ("Failed to open file for writing: " + filepath);
    }

    // Set internal I/O buffer as a multiple of the actual (not requested) write buffer size, making sure to clamp at file size.
    const size_t actualBufferSize {bufferSizeInBytes (requestedWriteBufferSize, samples.size(), bitsPerSample)};
    const size_t totalBytesToWrite {samples.size() * (bitsPerSample / 8)};
    const size_t ioBufferSize {std::min (ioBufferSizeMultiplier * actualBufferSize, totalBytesToWrite)};
    if (setvbuf (out.get(), nullptr, _IOFBF, ioBufferSize) != 0)
    {
        throw WriteError ("Failed to set file buffer.");
    }

    WriteHeader (out.get(), bitsPerSample, formatCode, sampleRate, channels, samples.size());
    WriteData (out.get(), samples, bitsPerSample, formatCode, requestedWriteBufferSize);

    // If all goes well, close the file manually so we can throw an exception if it fails.
    if (fclose (out.release()) != 0)
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
