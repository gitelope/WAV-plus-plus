
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
requires (std::integral<T>)
void WriteToFile (std::ofstream & out, T value, int size = sizeof(value))
{
    assert (size > 0 && size <= sizeof(value));
    std::array<std::byte, sizeof(T)> bytes; // using sizeof(T) means that size is 4 for 24-bit samples
    for (int i=0; i<size; ++i)
    {
        bytes[i] = static_cast<std::byte> (value >> (8*i) & 0xFF);
    }
    out.write (reinterpret_cast<const char *> (bytes.data()), size);
    if (!out)
    {
        throw WriteError ("Failed to write to file.");
    }
}

template <FloatOrDouble T>
void WriteToFile (std::ofstream & out, T value)
{
    std::array<std::byte, sizeof(T)> bytes;
    // Convert float/double to integer so we can bit-shift it.
    auto intValue = std::bit_cast<std::conditional_t<sizeof(T) == 4, uint32_t, uint64_t>>(value);
    for (int i = 0; i < sizeof(T); ++i)
    {
        bytes[i] = static_cast<std::byte> (intValue >> (8*i) & 0xFF);
    }
    out.write (reinterpret_cast<const char *> (bytes.data()), bytes.size());
    if (!out)
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
    T offset {0};
    if (bitsPerSample == 8)
    {
        offset = 128;
    }
    const auto bytesPerSample {bitsPerSample / 8};
    // 2^(bits-1) - 1
    const auto max {static_cast<T> ((1 << (bitsPerSample - 1)) - 1)};
    const auto min {-max};
    switch (bitsPerSample)
    {
        case 8:
            for (size_t n = 0; n < samplesToWrite; ++n)
            {
                buffer [n] = static_cast<std::byte> (clamped (round (samples [n + readPosition] * max /* + dither */), min, max) + offset);
            }
            break;
        case 16:
            for (size_t n = 0; n < samplesToWrite; ++n)
            {
                const auto quantizedSample = static_cast<int16_t> (clamped (round (samples [n + readPosition] * max /* + dither */), min, max) + offset);
                const size_t i {n * bytesPerSample};
                buffer [i]   = static_cast<std::byte> (quantizedSample         & 0xFF);
                buffer [i+1] = static_cast<std::byte> ((quantizedSample >> 8)  & 0xFF);
            }
            break;
        case 24:
            for (size_t n = 0; n < samplesToWrite; ++n)
            {
                const auto quantizedSample = static_cast<int32_t> (clamped (round (samples [n + readPosition] * max /* + dither */), min, max) + offset);
                const size_t i {n * bytesPerSample};
                buffer [i]   = static_cast<std::byte> (quantizedSample          & 0xFF);
                buffer [i+1] = static_cast<std::byte> ((quantizedSample >> 8)   & 0xFF);
                buffer [i+2] = static_cast<std::byte> ((quantizedSample >> 16)  & 0xFF);
            }
            break;
        case 32:
            for (size_t n = 0; n < samplesToWrite; ++n)
            {
                const auto quantizedSample = static_cast<int32_t> (clamped (round (samples [n + readPosition] * max /* + dither */), min, max) + offset);
                const size_t i {n * bytesPerSample};
                buffer [i]   = static_cast<std::byte> (quantizedSample          & 0xFF);
                buffer [i+1] = static_cast<std::byte> ((quantizedSample >> 8)   & 0xFF);
                buffer [i+2] = static_cast<std::byte> ((quantizedSample >> 16)  & 0xFF);
                buffer [i+3] = static_cast<std::byte> ((quantizedSample >> 24)  & 0xFF);
            }
            break;
    }
}

template <FloatOrDouble T>
void WriteDataBuffered (std::ofstream & out, std::vector<T> const & samples,
                        uint16_t bitsPerSample, uint16_t formatCode, size_t requestedBufferSizeInBytes)
{
    // first attempt: use a std::vector<char> for all types

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

        if (!out.write (reinterpret_cast<const char *> (buffer.data()), bytesToWrite))
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
        if (!out.write (reinterpret_cast<const char *> (&padding), sizeof (padding)))
        {
            throw WriteError ("Failed to write padding byte.");
        }
    }
}

} // namespace wav

#endif
