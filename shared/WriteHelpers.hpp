
#ifndef wavplusplus_WriteHelpers_hpp
#define wavplusplus_WriteHelpers_hpp

#include "Formats.hpp"
#include "Exceptions.hpp"
#include <vector>
#include <string>
#include <cstdio>
#include <cassert>

namespace wav
{

std::pair<uint16_t,uint16_t> bitDepthAndFormatCode (ExportFormat ef);
void WriteHeader (FILE * out, uint16_t bitsPerSample, uint16_t formatCode, int sampleRate, int channels, size_t numSamples);
void WriteToFile (FILE * out, std::string const & str);
template <FloatOrDouble T> void WriteLPCMU8   (FILE * out, std::vector<T> const & samples, size_t bufferSizeInBytes);
template <FloatOrDouble T> void WriteLPCM16LE (FILE * out, std::vector<T> const & samples, size_t bufferSizeInBytes);
template <FloatOrDouble T> void WriteLPCM32LE (FILE * out, std::vector<T> const & samples, size_t bufferSizeInBytes);

// Write integer or float.
//
// By default, the number of bytes is the container size,
// but it may also be less than the container size.
template <typename T>
requires (std::integral<T>)
void WriteToFile (FILE * out, T value, int size = sizeof (value))
{
    assert (size > 0 && size <= sizeof (value));
    std::array<std::byte, sizeof (T)> bytes; // using sizeof(T) means that size is 4 for 24-bit samples
    for (int i=0; i<size; ++i)
    {
        bytes[i] = static_cast<std::byte> (value >> (8*i) & 0xFF);
    }
    if (fwrite (bytes.data(), 1, size, out) != static_cast<size_t>(size))
    {
        throw WriteError ("Failed to write to file.");
    }
}

template <FloatOrDouble T>
void WriteToFile (FILE * out, T value)
{
    std::array<std::byte, sizeof(T)> bytes;
    // Convert float/double to integer so we can bit-shift it.
    auto intValue = std::bit_cast<std::conditional_t<sizeof(T) == 4, uint32_t, uint64_t>>(value);
    for (int i = 0; i < sizeof(T); ++i)
    {
        bytes[i] = static_cast<std::byte> (intValue >> (8*i) & 0xFF);
    }
    if (fwrite (bytes.data(), 1, bytes.size(), out) != bytes.size())
    {
        throw WriteError ("Failed to write to file.");
    }
}

template <FloatOrDouble T>
T clamped (T value, T min, T max)
{
    return std::min (std::max (min, value), max);
}

template <FloatOrDouble T>
void FillBufferFloatPCM (std::vector<T> const & samples, std::vector<std::byte> & buffer,
                         size_t readPosition, size_t samplesToWrite, uint16_t bitsPerSample)
{
    const auto bytesPerSample {bitsPerSample / 8};
    switch (bitsPerSample)
    {
        case 32:
            for (size_t n = 0; n < samplesToWrite; ++n)
            {
                const auto current = static_cast<float> (samples [n+readPosition]);
                const auto i {n * bytesPerSample};
                const auto integer = std::bit_cast<uint32_t> (current);
                buffer [i]   = static_cast<std::byte> (integer         & 0xFF);
                buffer [i+1] = static_cast<std::byte> ((integer >> 8)  & 0xFF);
                buffer [i+2] = static_cast<std::byte> ((integer >> 16) & 0xFF);
                buffer [i+3] = static_cast<std::byte> ((integer >> 24) & 0xFF);
            }
            break;
        case 64:
            for (size_t n = 0; n < samplesToWrite; ++n)
            {
                const auto current = static_cast<double> (samples [n+readPosition]);
                const auto i {n * bytesPerSample};
                const auto integer = std::bit_cast<uint64_t>(current);
                buffer [i]   = static_cast<std::byte> (integer         & 0xFF);
                buffer [i+1] = static_cast<std::byte> ((integer >> 8)  & 0xFF);
                buffer [i+2] = static_cast<std::byte> ((integer >> 16) & 0xFF);
                buffer [i+3] = static_cast<std::byte> ((integer >> 24) & 0xFF);
                buffer [i+4] = static_cast<std::byte> ((integer >> 32) & 0xFF);
                buffer [i+5] = static_cast<std::byte> ((integer >> 40) & 0xFF);
                buffer [i+6] = static_cast<std::byte> ((integer >> 48) & 0xFF);
                buffer [i+7] = static_cast<std::byte> ((integer >> 56) & 0xFF);
            }
            break;
    }
}

template <FloatOrDouble T>
void FillBufferLinearPCM (std::vector<T> const & samples, std::vector<std::byte> & buffer,
                          size_t readPosition, size_t samplesToWrite, uint16_t bitsPerSample)
{
    const auto max {static_cast<T> ((1 << (bitsPerSample - 1)) - 1)}; // 2^(bits-1) - 1
    const auto min {-max};
    auto quantized = [&min, &max] (T sample) -> T
    {
        return clamped (std::round (sample * max /* + dither */), min, max);
    };

    const auto bytesPerSample {bitsPerSample / 8};

    // Quantize samples and write their bytes to buffer in little-endian order:
    switch (bitsPerSample)
    {
        case 16:
            for (size_t n = 0; n < samplesToWrite; ++n)
            {
                const auto quantizedSample = static_cast<int16_t> (quantized (samples [n+readPosition]));

                const auto i {n * bytesPerSample};
                buffer [i]   = static_cast<std::byte> (quantizedSample        & 0xFF);
                buffer [i+1] = static_cast<std::byte> ((quantizedSample >> 8) & 0xFF);
            }
            break;
        case 24:
            for (size_t n = 0; n < samplesToWrite; ++n)
            {
                const auto quantizedSample = static_cast<int32_t> (quantized (samples [n+readPosition]));

                const auto i {n * bytesPerSample};
                buffer [i]   = static_cast<std::byte> (quantizedSample         & 0xFF);
                buffer [i+1] = static_cast<std::byte> ((quantizedSample >> 8)  & 0xFF);
                buffer [i+2] = static_cast<std::byte> ((quantizedSample >> 16) & 0xFF);
            }
            break;
        case 32:
            for (size_t n = 0; n < samplesToWrite; ++n)
            {
                const auto quantizedSample = static_cast<int32_t> (quantized (samples [n+readPosition]));

                const size_t i {n * bytesPerSample};
                buffer [i]   = static_cast<std::byte> (quantizedSample         & 0xFF);
                buffer [i+1] = static_cast<std::byte> ((quantizedSample >> 8)  & 0xFF);
                buffer [i+2] = static_cast<std::byte> ((quantizedSample >> 16) & 0xFF);
                buffer [i+3] = static_cast<std::byte> ((quantizedSample >> 24) & 0xFF);
            }
            break;
    }
}

template <FloatOrDouble T>
void WriteFloatToFloatLE (FILE * out, std::vector<T> const & samples, size_t bufferSizeInBytes, uint16_t bitsPerSample)
{
    // Output bit depth same as input.
    if ((std::is_same_v<T, float> && bitsPerSample == 32) || (std::is_same_v<T, double> && bitsPerSample == 64))
    {
        // No need for any conversion. Write data straight to file.
        fwrite (samples.data(), sizeof (T), samples.size(), out);
    }
    else if (std::is_same_v<T, double> && bitsPerSample == 32)
    {
        // double -> float
        std::vector<float> buffer (bufferSizeInBytes / 4);

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
        std::vector<double> buffer (bufferSizeInBytes / 8);

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

template <FloatOrDouble T>
void WriteData (FILE * out, std::vector<T> const & samples,
                uint16_t bitsPerSample, uint16_t formatCode, size_t requestedBufferSizeInBytes)
{
    assert (bitsPerSample % 8 == 0);
    const auto numSamples {samples.size()};
    const auto sampleSizeInBytes {static_cast<size_t> (bitsPerSample / 8)};
    const auto totalBytesToWrite {numSamples * sampleSizeInBytes};

    // Determine buffer size from requested size, sample size, and number of samples:
    // 1. Buffer must be able to hold an entire sample's worth of bytes.
    size_t bufferSizeInBytes = std::max (requestedBufferSizeInBytes, sampleSizeInBytes);
    // 2. Buffer size must correspond to a whole number of samples.
    bufferSizeInBytes = (bufferSizeInBytes / sampleSizeInBytes) * sampleSizeInBytes;
    // 3. Buffer size must not exceed the total number of bytes to write.
    bufferSizeInBytes = std::min (bufferSizeInBytes, totalBytesToWrite);

    // Input and output endianness are the same.
    if constexpr (std::endian::native == std::endian::little)
    {
        if (formatCode == FormatCode::FloatPCM)
        {
            // In benchmarking on macOS Sequoia, a significant speed-up was observed
            // when saving to 64-bit float with WriteFloatToFloatLE() over using the
            // fallback path below. (No change was shown when saving to 32-bit float.)
            WriteFloatToFloatLE (out, samples, bufferSizeInBytes, bitsPerSample);
            return;
        }
        else if (formatCode == FormatCode::LinearPCM)
        {
            switch (bitsPerSample)
            {
                case 8:
                {
                    WriteLPCMU8 (out, samples, bufferSizeInBytes);
                    return;
                }
                case 16:
                {
                    WriteLPCM16LE (out, samples, bufferSizeInBytes);
                    return;
                }
                case 32:
                {
                    WriteLPCM32LE (out, samples, bufferSizeInBytes);
                    return;
                }
            }
        }
    }
    // system is not little-endian
    else if (bitsPerSample == 8 && formatCode == FormatCode::LinearPCM)
    {
        WriteLPCMU8 (out, samples, bufferSizeInBytes);
        return;
    }

    // The fallback path:
    std::vector<std::byte> buffer (bufferSizeInBytes);

    size_t samplesRemaining {numSamples};
    size_t readPosition {0};

    while (samplesRemaining)
    {
        const auto bytesToWrite {std::min (samplesRemaining * sampleSizeInBytes, buffer.size())};
        const auto samplesToWrite {bytesToWrite / sampleSizeInBytes};
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
void WriteLPCMU8 (FILE * out, std::vector<T> const & samples, size_t bufferSizeInBytes)
{
    constexpr size_t bytesPerSample {1};
    const auto max {static_cast<T>(0x7F)}; // 2^7 - 1 = 127
    const auto min {-max};
    const auto offset {static_cast<T>(0x80)}; // 2^7 = 128

    std::vector<uint8_t> buffer (bufferSizeInBytes);

    size_t samplesRemaining {samples.size()};
    size_t readPosition {0};

    while (samplesRemaining)
    {
        const auto samplesToWrite {std::min (samplesRemaining, buffer.size())};
        for (size_t i=0; i < samplesToWrite; ++i)
        {
            buffer [i] = static_cast<uint8_t> (clamped (std::round (samples [readPosition + i] * max /* + dither */), min, max) + offset);
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
void WriteLPCM16LE (FILE * out, std::vector<T> const & samples, size_t bufferSizeInBytes)
{
    constexpr size_t bytesPerSample {2};
    const auto max {static_cast<T>(0x7FFF)}; // 2^15 - 1 = 32,767
    const auto min {-max};

    std::vector<int16_t> buffer (bufferSizeInBytes / bytesPerSample);

    size_t samplesRemaining {samples.size()};
    size_t readPosition {0};

    while (samplesRemaining)
    {
        const auto samplesToWrite {std::min (samplesRemaining, buffer.size())};
        for (size_t i=0; i < samplesToWrite; ++i)
        {
            buffer [i] = static_cast<int16_t> (clamped (std::round (samples [readPosition + i] * max /* + dither */), min, max));
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
void WriteLPCM32LE (FILE * out, std::vector<T> const & samples, size_t bufferSizeInBytes)
{
    constexpr size_t bytesPerSample {4};
    const auto max {static_cast<T>(0x7FFFFFFF)}; // 2^31 - 1 = 2,147,483,647
    const auto min {-max};

    std::vector<int32_t> buffer (bufferSizeInBytes / bytesPerSample);

    size_t samplesRemaining {samples.size()};
    size_t readPosition {0};

    while (samplesRemaining)
    {
        const auto samplesToWrite {std::min (samplesRemaining, buffer.size())};
        for (size_t i=0; i < samplesToWrite; ++i)
        {
            buffer [i] = static_cast<int32_t> (clamped (std::round (samples [readPosition + i] * max /* + dither */), min, max));
        }
        if (fwrite (buffer.data(), bytesPerSample, samplesToWrite, out) != samplesToWrite)
        {
            throw WriteError ("Failed to write buffer to file.");
        }
        readPosition += samplesToWrite;
        samplesRemaining -= samplesToWrite;
    }
}

} // namespace wav

#endif

