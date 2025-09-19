
#ifndef wavplusplus_Write_hpp
#define wavplusplus_Write_hpp

#include "WriteHelpers.hpp"
#include <cstdio>
#include <span>
#include <vector>
#include <array>

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

// Buffering
//
// 32kB default (requested) buffer size
inline constexpr size_t defaultRequestedWriteBufferSize {32768};
// Internal I/O buffer size will be this multiple of the actual–not requested–write buffer size.
inline constexpr size_t defaultIOBufferSizeMultiplier {8};
// When the system is little-endian and the input and output formats match,
// i.e. writing floats to Float32 or doubles to Float64, neither will a write
// buffer will be used–all the data will be written in a single write–nor
// will setvbuf() be called to adjust the internal I/O buffer size.
// requestedWriteBufferSize and ioBufferSizeMultiplier will be ignored.

/// Write a std::vector.
/// (May throw wav::WriteError.)
template <FloatOrDouble T>
void Write (std::string const & filepath,
            std::vector<T> const & samples,
            ExportFormat exportFormat,
            uint32_t sampleRate, uint16_t channels,
            // requested write buffer size used by WAV++
            size_t requestedWriteBufferSize = defaultRequestedWriteBufferSize,
            // internal IO buffer size used by OS is a multiple of actual–not requested–write buffer size
            size_t ioBufferSizeMultiplier = defaultIOBufferSizeMultiplier);

/// Write a C-style array.
/// (May throw wav::WriteError.)
template <FloatOrDouble T, size_t N>
void Write (std::string const & filepath,
            const T (&samples)[N],
            ExportFormat exportFormat,
            uint32_t sampleRate, uint16_t channels,
            // requested write buffer size used by WAV++
            size_t requestedWriteBufferSize = defaultRequestedWriteBufferSize,
            // internal IO buffer size used by OS is a multiple of actual–not requested–write buffer size
            size_t ioBufferSizeMultiplier = defaultIOBufferSizeMultiplier);

/// Write a std::array.
/// (May throw wav::WriteError.)
template <FloatOrDouble T, size_t N>
void Write (std::string const & filepath,
            std::array<T,N> const & samples,
            ExportFormat exportFormat,
            uint32_t sampleRate, uint16_t channels,
            // requested write buffer size used by WAV++
            size_t requestedWriteBufferSize = defaultRequestedWriteBufferSize,
            // internal IO buffer size used by OS is a multiple of actual–not requested–write buffer size
            size_t ioBufferSizeMultiplier = defaultIOBufferSizeMultiplier);

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

/////////////////////////////////////////////////////////////////////////
// std::span overload does all the work.
// (May throw wav::WriteError.)
template <FloatOrDouble T>
void Write (std::string const & filepath, std::span<const T> samples,
            ExportFormat exportFormat,
            uint32_t sampleRate, uint16_t channels,
            // requested write buffer size used by WAV++
            size_t requestedWriteBufferSize,
            // internal IO buffer size used by OS is a multiple of actual–not requested–write buffer size
            size_t ioBufferSizeMultiplier)
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

    WriteHeader (out.get(), bitsPerSample, formatCode, sampleRate, channels, samples.size());
    WriteData (out.get(), samples, bitsPerSample, formatCode, requestedWriteBufferSize, ioBufferSizeMultiplier);

    // If all goes well, close the file manually so we can throw an exception if it fails.
    if (fclose (out.release()) != 0)
    {
        throw WriteError ("File did not close successfully.");
    }
}

/////////////////////////////////////////////////////////////////////
// convenience overloads defined

// std::vector overload calls std::span overload.
// (May throw wav::WriteError.)
template <FloatOrDouble T>
void Write (std::string const & filepath, std::vector<T> const & samples,
            ExportFormat exportFormat,
            uint32_t sampleRate, uint16_t channels,
            size_t requestedWriteBufferSize, size_t ioBufferSizeMultiplier)
{
    Write (filepath, std::span<const T> (samples), exportFormat, sampleRate, channels, requestedWriteBufferSize, ioBufferSizeMultiplier);
}

// C-array overload calls std::span overload.
// (May throw wav::WriteError.)
template <FloatOrDouble T, size_t N>
void Write (std::string const & filepath, const T (&samples)[N],
            ExportFormat exportFormat,
            uint32_t sampleRate, uint16_t channels,
            size_t requestedWriteBufferSize, size_t ioBufferSizeMultiplier)
{
    Write (filepath, std::span<const T> (samples), exportFormat, sampleRate, channels, requestedWriteBufferSize, ioBufferSizeMultiplier);
}

// std::array overload calls std::span overload.
// (May throw wav::WriteError.)
template <FloatOrDouble T, size_t N>
void Write (std::string const & filepath, std::array<T,N> const & samples,
            ExportFormat exportFormat,
            uint32_t sampleRate, uint16_t channels,
            size_t requestedWriteBufferSize, size_t ioBufferSizeMultiplier)
{
    Write (filepath, std::span<const T> (samples), exportFormat, sampleRate, channels, requestedWriteBufferSize, ioBufferSizeMultiplier);
}

} // namespace wav

#endif
