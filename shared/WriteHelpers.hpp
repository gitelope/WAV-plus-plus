
#ifndef wavplusplus_WriteHelpers_hpp
#define wavplusplus_WriteHelpers_hpp

#include "Formats.hpp"
#include "Exceptions.hpp"
#include "int24.hpp"
#include "BufferSizing.hpp"
#include <vector>
#include <string>
#include <cstdio>
#include <cassert>
#include <span>

namespace wav
{

// header
void WriteHeader (FILE * out, int bitsPerSample, uint16_t formatCode, uint32_t sampleRate, int channels, size_t numSamples);
void WriteToHeader (FILE * out, std::string const & str);

template <FloatOrDouble T> void WriteData (FILE * out, std::span<const T> samples, int bitsPerSample, uint16_t formatCode, size_t requestedWriteBufferSizeInBytes, size_t ioBufferSizeMultiplier);

// data on LE systems
template <FloatOrDouble T> void WriteDataLE (FILE * out, std::span<const T> samples, int bitsPerSample, uint16_t formatCode, size_t writeBufferSizeInBytes, size_t ioBufferSizeInBytes);
template <FloatOrDouble T> void WriteFloatToFloatLE (FILE * out, std::span<const T> samples, int bitsPerSample, size_t writeBufferSizeInBytes, size_t ioBufferSizeInBytes);
template <FloatOrDouble T> void WriteLPCM16LE (FILE * out, std::span<const T> samples, size_t writeBufferSizeInBytes, size_t ioBufferSizeInBytes);
template <FloatOrDouble T> void WriteLPCM24LE (FILE * out, std::span<const T> samples, size_t writeBufferSizeInBytes, size_t ioBufferSizeInBytes);
template <FloatOrDouble T> void WriteLPCM32LE (FILE * out, std::span<const T> samples, size_t writeBufferSizeInBytes, size_t ioBufferSizeInBytes);

// 8-bit LPCM data (LE and BE)
template <FloatOrDouble T> void WriteLPCMU8 (FILE * out, std::span<const T> samples, size_t writeBufferSizeInBytes, size_t ioBufferSizeInBytes);

// data on BE systems (also works for LE but not optimal)
template <FloatOrDouble T> void WriteDataFallback (FILE * out, std::span<const T> samples, int bitsPerSample, uint16_t formatCode, size_t writeBufferSizeInBytes, size_t ioBufferSizeInBytes);
template <FloatOrDouble T> void FillBufferLinearPCM (std::span<const T> samples, std::vector<std::byte> & buffer, size_t readPosition, size_t samplesToWrite, int bitsPerSample);
template <FloatOrDouble T> void FillBufferFloatPCM (std::span<const T> samples, std::vector<std::byte> & buffer, size_t readPosition, size_t samplesToWrite, int bitsPerSample);

std::pair<int,uint16_t> bitDepthAndFormatCode (ExportFormat ef);

template <FloatOrDouble T> T quantized (T value, T min, T max)
{
    // Quantize floating-point value to signed integer (still in the form of a float or double).
    // Assumes scaling factor and clamping maximum are one and the same.
    const T scaled {value * max};
    /* dither */
    const T rounded {std::round (scaled)};
    return std::clamp (rounded, min, max);
}

// Write integer as header field. Not for writing data.
template <std::integral T>
void WriteToHeader (FILE * out, T value)
{
    std::array<std::byte, sizeof(T)> bytes;
    for (size_t i=0; i < bytes.size(); ++i)
    {
        bytes[i] = static_cast<std::byte> (value >> (8*i));
    }
    if (fwrite (bytes.data(), 1, bytes.size(), out) != bytes.size())
    {
        throw WriteError ("Failed to write to file.");
    }
}

template <FloatOrDouble T>
void WriteData (FILE * out, std::span<const T> samples,
                int bitsPerSample, uint16_t formatCode,
                size_t requestedWriteBufferSizeInBytes, size_t ioBufferSizeMultiplier)
{
    // A write buffer is used in all cases except for writing floating-point samples to the same format on LE systems.
    const size_t writeBufferSize {bufferSizeInBytes (requestedWriteBufferSizeInBytes, samples.size(), bitsPerSample)};
    const size_t ioBufferSize {ioBufferSizeInBytes (samples.size(), bitsPerSample, writeBufferSize, ioBufferSizeMultiplier)};

    // Input and output endianness are the same.
    if constexpr (std::endian::native == std::endian::little)
    {
        WriteDataLE (out, samples, bitsPerSample, formatCode, writeBufferSize, ioBufferSize);
    }
    // System is not little-endian.
    else if (bitsPerSample == 8 && formatCode == FormatCode::LinearPCM)
    {
        WriteLPCMU8 (out, samples, writeBufferSize, ioBufferSize); // Since samples are one byte, this is also used for LE.
    }
    else
    {
        WriteDataFallback (out, samples, bitsPerSample, formatCode, writeBufferSize, ioBufferSize);
    }
}

template <FloatOrDouble T>
void WriteDataLE (FILE * out, std::span<const T> samples,
                  int bitsPerSample, uint16_t formatCode,
                  size_t writeBufferSizeInBytes, size_t ioBufferSizeInBytes)
{
    if (formatCode == FormatCode::FloatPCM)
    {
        WriteFloatToFloatLE (out, samples, bitsPerSample, writeBufferSizeInBytes, ioBufferSizeInBytes);
    }
    else if (formatCode == FormatCode::LinearPCM)
    {
        switch (bitsPerSample)
        {
            case 8:  WriteLPCMU8   (out, samples, writeBufferSizeInBytes, ioBufferSizeInBytes); break;
            case 16: WriteLPCM16LE (out, samples, writeBufferSizeInBytes, ioBufferSizeInBytes); break;
            case 24: WriteLPCM24LE (out, samples, writeBufferSizeInBytes, ioBufferSizeInBytes); break;
            case 32: WriteLPCM32LE (out, samples, writeBufferSizeInBytes, ioBufferSizeInBytes); break;
        }
    }
}

template <FloatOrDouble T>
void WriteFloatToFloatLE (FILE * out, std::span<const T> samples, int bitsPerSample, size_t writeBufferSizeInBytes, size_t ioBufferSizeInBytes)
{
    // In benchmarking on macOS Sequoia, a significant speed-up was observed
    // when saving to 64-bit float with WriteFloatToFloatLE() over using the
    // fallback path. (No change was shown when saving to 32-bit float.)

    // Output bit depth same as input.
    if ((std::is_same_v<T, float> && bitsPerSample == 32) || (std::is_same_v<T, double> && bitsPerSample == 64))
    {
        // No need for any conversion. Write data straight to file.
        // (Also, no write buffer and no adjusting internal I/O buffer with setvbuf().)
        if (fwrite (samples.data(), sizeof (T), samples.size(), out) != samples.size())
        {
            throw WriteError ("Failed to write buffer to file.");
        }
    }
    else if (std::is_same_v<T, double> && bitsPerSample == 32)
    {
        // double -> float
        std::vector<float> buffer (writeBufferSizeInBytes / 4);
        SetInternalIOBufferSize<WriteError> (out, ioBufferSizeInBytes);

        size_t samplesRemaining {samples.size()};
        size_t readPosition {0};
        while (samplesRemaining)
        {
            const auto samplesToWrite {std::min (samplesRemaining, buffer.size())};
            for (size_t i=0; i < samplesToWrite; ++i)
            {
                buffer [i] = static_cast<float> (samples [readPosition + i]);
            }
            if (fwrite (buffer.data(), 4, samplesToWrite, out) != samplesToWrite)
            {
                throw WriteError ("Failed to write buffer to file.");
            }
            readPosition += samplesToWrite;
            samplesRemaining -= samplesToWrite;
        }
    }
    else if (std::is_same_v<T, float> && bitsPerSample == 64)
    {
        // float -> double
        std::vector<double> buffer (writeBufferSizeInBytes / 8);
        SetInternalIOBufferSize<WriteError> (out, ioBufferSizeInBytes);

        size_t samplesRemaining {samples.size()};
        size_t readPosition {0};
        while (samplesRemaining)
        {
            const auto samplesToWrite {std::min (samplesRemaining, buffer.size())};
            for (size_t i=0; i < samplesToWrite; ++i)
            {
                buffer [i] = static_cast<double> (samples [readPosition + i]);
            }
            if (fwrite (buffer.data(), 8, samplesToWrite, out) != samplesToWrite)
            {
                throw WriteError ("Failed to write buffer to file.");
            }
            readPosition += samplesToWrite;
            samplesRemaining -= samplesToWrite;
        }
    }
}

// Both LE and BE since samples are a single byte in size.
template <FloatOrDouble T>
void WriteLPCMU8 (FILE * out, std::span<const T> samples, size_t writeBufferSizeInBytes, size_t ioBufferSizeInBytes)
{
    constexpr size_t bytesPerSample {1};
    const auto max {static_cast<T> (0x7F)}; // 2^7 - 1 = 127
    const auto min {-max};
    const auto offset {static_cast<T> (0x80)}; // 2^7 = 128

    std::vector<std::byte> buffer (writeBufferSizeInBytes);
    SetInternalIOBufferSize<WriteError> (out, ioBufferSizeInBytes);

    size_t samplesRemaining {samples.size()};
    size_t readPosition {0};

    while (samplesRemaining)
    {
        const auto samplesToWrite {std::min (samplesRemaining, buffer.size())};
        for (size_t i=0; i < samplesToWrite; ++i)
        {
            const auto scaled = samples [readPosition + i] * max;
            /* dither */
            const auto rounded {std::round (scaled)};
            const auto clamped {std::clamp (rounded, min, max)};
            const auto quantized {clamped + offset};
            buffer [i] = static_cast<std::byte> (quantized);
        }
        if (fwrite (buffer.data(), bytesPerSample, samplesToWrite, out) != samplesToWrite)
        {
            throw WriteError ("Failed to write buffer to file.");
        }
        readPosition += samplesToWrite;
        samplesRemaining -= samplesToWrite;
    }

    if (samples.size() % 2) // samples.size() is the total number of bytes since each sample is 1 byte for 8-bit
    {
        // If an odd number of data bytes, add a padding byte.
        constexpr auto padding {std::byte {0x00}};
        if (fwrite (&padding, 1, 1, out) != 1)
        {
            throw WriteError ("Failed to write padding byte.");
        }
    }
}

template <FloatOrDouble T>
void WriteLPCM16LE (FILE * out, std::span<const T> samples, size_t writeBufferSizeInBytes, size_t ioBufferSizeInBytes)
{
    constexpr size_t bytesPerSample {2};
    const auto max {static_cast<T> (0x7FFF)}; // 2^15 - 1 = 32,767
    const auto min {-max};

    std::vector<int16_t> buffer (writeBufferSizeInBytes / bytesPerSample);
    SetInternalIOBufferSize<WriteError> (out, ioBufferSizeInBytes);

    size_t samplesRemaining {samples.size()};
    size_t readPosition {0};

    while (samplesRemaining)
    {
        const auto samplesToWrite {std::min (samplesRemaining, buffer.size())};
        for (size_t i=0; i < samplesToWrite; ++i)
        {
            buffer [i] = static_cast<int16_t> (quantized (samples [readPosition + i], min, max));
        }
        if (fwrite (buffer.data(), bytesPerSample, samplesToWrite, out) != samplesToWrite)
        {
            throw WriteError ("Failed to write buffer to file.");
        }
        readPosition += samplesToWrite;
        samplesRemaining -= samplesToWrite;
    }
}

template <FloatOrDouble T>
void WriteLPCM24LE (FILE * out, std::span<const T> samples, size_t writeBufferSizeInBytes, size_t ioBufferSizeInBytes)
{
    constexpr size_t bytesPerSample {3};
    const auto max {static_cast<T> (0x7FFFFF)}; // 2^23 - 1 = 8,388,607
    const auto min {-max};

    std::vector<int24> buffer (writeBufferSizeInBytes / bytesPerSample);
    SetInternalIOBufferSize<WriteError> (out, ioBufferSizeInBytes);

    size_t samplesRemaining {samples.size()};
    size_t readPosition {0};

    while (samplesRemaining)
    {
        const auto samplesToWrite {std::min (samplesRemaining, buffer.size())};
        for (size_t i = 0; i < samplesToWrite; ++i)
        {
            buffer[i] = static_cast<int24> (quantized (samples [readPosition + i], min, max));
        }

        if (fwrite (buffer.data(), bytesPerSample, samplesToWrite, out) != samplesToWrite)
        {
            throw WriteError ("Failed to write buffer to file.");
        }
        readPosition += samplesToWrite;
        samplesRemaining -= samplesToWrite;
    }

    if (samples.size() % 2) // If samples is odd, then total bytes is odd since bytesPerSample is 3 an therefore odd.
    {
        // If an odd number of data bytes, add a padding byte.
        constexpr auto padding {std::byte {0x00}};
        if (fwrite (&padding, 1, 1, out) != 1)
        {
            throw WriteError ("Failed to write padding byte.");
        }
    }
}

template <FloatOrDouble T>
void WriteLPCM32LE (FILE * out, std::span<const T> samples, size_t writeBufferSizeInBytes, size_t ioBufferSizeInBytes)
{
    constexpr size_t bytesPerSample {4};
    const auto max {static_cast<T> (0x7FFFFFFF)}; // 2^31 - 1 = 2,147,483,647
    const auto min {-max};

    std::vector<int32_t> buffer (writeBufferSizeInBytes / bytesPerSample);
    SetInternalIOBufferSize<WriteError> (out, ioBufferSizeInBytes);

    size_t samplesRemaining {samples.size()};
    size_t readPosition {0};

    while (samplesRemaining)
    {
        const auto samplesToWrite {std::min (samplesRemaining, buffer.size())};
        for (size_t i=0; i < samplesToWrite; ++i)
        {
            buffer [i] = static_cast<int32_t> (quantized (samples [readPosition + i], min, max));
        }
        if (fwrite (buffer.data(), bytesPerSample, samplesToWrite, out) != samplesToWrite)
        {
            throw WriteError ("Failed to write buffer to file.");
        }
        readPosition += samplesToWrite;
        samplesRemaining -= samplesToWrite;
    }
}

template <FloatOrDouble T>
void WriteDataFallback (FILE * out, std::span<const T> samples,
                        int bitsPerSample, uint16_t formatCode,
                        size_t writeBufferSizeInBytes, size_t ioBufferSizeInBytes)
{
    assert (bitsPerSample % 8 == 0);
    const auto numSamples {samples.size()};
    const auto bytesPerSamples {static_cast<size_t> (bitsPerSample / 8)};
    const auto totalBytesToWrite {numSamples * bytesPerSamples};

    std::vector<std::byte> buffer (writeBufferSizeInBytes);
    SetInternalIOBufferSize<WriteError> (out, ioBufferSizeInBytes);

    size_t samplesRemaining {numSamples};
    size_t readPosition {0};

    while (samplesRemaining)
    {
        const auto bytesToWrite {std::min (samplesRemaining * bytesPerSamples, buffer.size())};
        const auto samplesToWrite {bytesToWrite / bytesPerSamples};
        // fill a buffer
        if      (formatCode == FormatCode::LinearPCM) FillBufferLinearPCM (samples, buffer, readPosition, samplesToWrite, bitsPerSample);
        else if (formatCode == FormatCode::FloatPCM)  FillBufferFloatPCM  (samples, buffer, readPosition, samplesToWrite, bitsPerSample);
        else assert (false);

        if (fwrite (buffer.data(), 1, bytesToWrite, out) != bytesToWrite)
        {
            throw WriteError ("Failed to write buffer to file.");
        }

        readPosition += samplesToWrite;
        samplesRemaining -= samplesToWrite;
    }
    if (totalBytesToWrite % 2)
    {
        // If an odd number of data bytes, add a padding byte.
        constexpr auto padding {std::byte {0x00}};
        if (fwrite (&padding, 1, 1, out) != 1)
        {
            throw WriteError ("Failed to write padding byte.");
        }
    }
}

template <FloatOrDouble T>
void FillBufferLinearPCM (std::span<const T> samples, std::vector<std::byte> & buffer,
                          size_t readPosition, size_t samplesToWrite, int bitsPerSample)
{
    const auto max {static_cast<T> ((1 << (bitsPerSample - 1)) - 1)}; // 2^(bits-1) - 1
    const auto min {-max};

    const auto bytesPerSample {static_cast<size_t> (bitsPerSample / 8)};

    // Quantize samples and write their bytes to buffer in little-endian order:
    switch (bitsPerSample)
    {
        case 16:
            for (size_t n = 0; n < samplesToWrite; ++n)
            {
                const auto quantizedSample = static_cast<int16_t> (quantized (samples [n+readPosition], min, max));

                const size_t i {n * bytesPerSample};
                buffer [i]   = static_cast<std::byte> (quantizedSample);
                buffer [i+1] = static_cast<std::byte> (quantizedSample >> 8);
            }
            break;
        case 24:
            for (size_t n = 0; n < samplesToWrite; ++n)
            {
                const auto quantizedSample = static_cast<int32_t> (quantized (samples [n+readPosition], min, max));

                const size_t i {n * bytesPerSample};
                buffer [i]   = static_cast<std::byte> (quantizedSample);
                buffer [i+1] = static_cast<std::byte> (quantizedSample >> 8);
                buffer [i+2] = static_cast<std::byte> (quantizedSample >> 16);
            }
            break;
        case 32:
            for (size_t n = 0; n < samplesToWrite; ++n)
            {
                const auto quantizedSample = static_cast<int32_t> (quantized (samples [n+readPosition], min, max));

                const size_t i {n * bytesPerSample};
                buffer [i]   = static_cast<std::byte> (quantizedSample);
                buffer [i+1] = static_cast<std::byte> (quantizedSample >> 8);
                buffer [i+2] = static_cast<std::byte> (quantizedSample >> 16);
                buffer [i+3] = static_cast<std::byte> (quantizedSample >> 24);
            }
            break;
    }
}

template <FloatOrDouble T>
void FillBufferFloatPCM (std::span<const T> samples, std::vector<std::byte> & buffer,
                         size_t readPosition, size_t samplesToWrite, int bitsPerSample)
{
    const auto bytesPerSample {static_cast<size_t> (bitsPerSample) / 8};
    switch (bitsPerSample)
    {
        case 32:
            for (size_t n = 0; n < samplesToWrite; ++n)
            {
                const auto current = static_cast<float> (samples [n+readPosition]);
                const size_t i {n * bytesPerSample};
                const auto integer = std::bit_cast<uint32_t> (current);
                buffer [i]   = static_cast<std::byte> (integer);
                buffer [i+1] = static_cast<std::byte> (integer >> 8);
                buffer [i+2] = static_cast<std::byte> (integer >> 16);
                buffer [i+3] = static_cast<std::byte> (integer >> 24);
            }
            break;
        case 64:
            for (size_t n = 0; n < samplesToWrite; ++n)
            {
                const auto current = static_cast<double> (samples [n+readPosition]);
                const size_t i {n * bytesPerSample};
                const auto integer = std::bit_cast<uint64_t>(current);
                buffer [i]   = static_cast<std::byte> (integer);
                buffer [i+1] = static_cast<std::byte> (integer >> 8);
                buffer [i+2] = static_cast<std::byte> (integer >> 16);
                buffer [i+3] = static_cast<std::byte> (integer >> 24);
                buffer [i+4] = static_cast<std::byte> (integer >> 32);
                buffer [i+5] = static_cast<std::byte> (integer >> 40);
                buffer [i+6] = static_cast<std::byte> (integer >> 48);
                buffer [i+7] = static_cast<std::byte> (integer >> 56);
            }
            break;
    }
}

/*
// Write float or double as header field. Not for writing data.
// (Could be used to write peak values to a PEAK chunk.)
template <FloatOrDouble T>
void WriteToHeader (FILE * out, T value)
{
    static_assert (sizeof (float) == 4, "floats must be 4 bytes.");
    static_assert (sizeof (double) == 8, "doubles must be 8 bytes.");
    static_assert (CHAR_BIT == 8, "The value of CHAR_BIT must be 8.");

    std::array<std::byte, sizeof(T)> bytes;
    // Convert float/double to integer so we can bit-shift it.
    const auto intValue = std::bit_cast<std::conditional_t<sizeof(T) == 4, uint32_t, uint64_t>> (value);
    for (int i = 0; i < bytes.size(); ++i)
    {
        bytes[i] = static_cast<std::byte> (intValue >> (8*i));
    }
    if (fwrite (bytes.data(), 1, bytes.size(), out) != bytes.size())
    {
        throw WriteError ("Failed to write to file.");
    }
}
 */

} // namespace wav

#endif

