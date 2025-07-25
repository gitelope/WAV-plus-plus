
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

template <FloatOrDouble T>
T clamped (T value, T min, T max)
{
    return std::min (std::max (min, value), max);
}

template <typename IntType, FloatOrDouble T>
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
template <typename IntType, FloatOrDouble T>
void WriteBulkLinearPCM (std::ofstream & out, std::vector<T> const & samples, uint16_t bitsPerSample)
{
    T offset {0};
    if constexpr (std::is_same_v <IntType, uint8_t>)
    {
        offset = 128;
    }
    // 2^(bits-1) - 1
    const auto max {static_cast<T> ((1 << (bitsPerSample - 1)) - 1)};
    const auto min {-max};

    if (bitsPerSample == 24)
    {
        // Requires special handling since we keep only the 3 least
        // significant bytes of an int32_t.
        constexpr size_t bytesPerSample {3};
        std::vector<char> bytes (bytesPerSample * samples.size());
        for (size_t n = 0; n < samples.size(); ++n)
        {
            // Note: Right-shifting a negative signed int in C++20 is guaranteed to
            // produce an arithmetic shift. The fact that the most significant bits are
            // sign-extended isn't important, but it's good to know the shift itself
            // is not implementation-defined. We need not cast to uint32_t before shifting.
            const auto quantizedSample = static_cast<int32_t> (clamped (round (samples [n] * max /* + dither */), min, max) + offset);
            const size_t i {n * bytesPerSample};
            bytes [i]   = static_cast<char> (quantizedSample         & 0xFF);
            bytes [i+1] = static_cast<char> ((quantizedSample >> 8)  & 0xFF);
            bytes [i+2] = static_cast<char> ((quantizedSample >> 16) & 0xFF);
        }
        if (!out.write (reinterpret_cast<const char*> (bytes.data()), bytes.size()))
        {
            throw WriteError ("Failed to write samples to file.");
        }
    }
    else // 8, 16, and 32 bits
    {
        std::vector<IntType> ints (samples.size());
        for (size_t n = 0; n < samples.size(); ++n)
        {
            ints [n] = static_cast<IntType> (clamped (round (samples [n] * max /* + dither */), min, max) + offset);
        }
        if (!out.write (reinterpret_cast<const char*> (ints.data()), ints.size() * sizeof(IntType)))
        {
            throw WriteError ("Failed to write samples to file.");
        }
    }
}

template <FloatOrDouble T>
void WriteData (std::ofstream & out, std::vector<T> const & samples, uint16_t bitsPerSample, uint16_t formatCode)
{
    if (formatCode == FormatCode::LinearPCM)
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
    else if (formatCode == FormatCode::FloatPCM)
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

template <FloatOrDouble T>
void WriteBulkLinearPCM (std::ofstream & out, std::vector<T> const & samples, uint16_t bitsPerSample)
{
    switch (bitsPerSample)
    {
        case 8:  WriteBulkLinearPCM<uint8_t> (out, samples, bitsPerSample);
            break;
        case 16: WriteBulkLinearPCM<int16_t> (out, samples, bitsPerSample);
            break;
        case 24: WriteBulkLinearPCM<int32_t> (out, samples, bitsPerSample);
            break;
        case 32: WriteBulkLinearPCM<int32_t> (out, samples, bitsPerSample);
            break;
    }
    // If an odd number of data bytes, add a padding byte.
    assert (bitsPerSample % 8 == 0);
    if (samples.size() * bitsPerSample / 8 % 2) out << static_cast<uint8_t> (0);
}

template <typename OutType, typename InType>
void WriteBulkFloatsAs (std::ofstream & out, std::vector<InType> const & samples)
{
    // Helper lambda to avoid redundancy.
    auto Write = [&out] (const auto & vec)
    {
        using ValueType = typename std::decay_t<decltype(vec)>::value_type;
        if (!out.write (reinterpret_cast<const char*> (vec.data()), vec.size() * sizeof (ValueType)))
        {
            throw WriteError ("Failed to write samples to file.");
        }
    };

    if constexpr (std::is_same_v<InType, OutType>)
    {
        Write (samples); // no conversion needed
    }
    else
    {
        Write (std::vector<OutType> (samples.begin(), samples.end())); // conversion to output type
    }
}

template <FloatOrDouble T>
void WriteBulkFloatPCM (std::ofstream & out, std::vector<T> const & samples, uint16_t bitsPerSample)
{
    static_assert (CHAR_BIT == 8, "A byte is not 8 bits.");
    static_assert (sizeof (float) == 4, "float is not 4 bytes.");
    static_assert (sizeof (double) == 8, "double is not 8 bytes.");

    switch (bitsPerSample)
    {
        case 32: WriteBulkFloatsAs<float>  (out, samples);   break;
        case 64: WriteBulkFloatsAs<double> (out, samples);   break;
        default: throw WriteError ("Unsupported floating-point bit depth.");
    }
}

template <FloatOrDouble T>
void WriteDataBulk (std::ofstream & out, std::vector<T> const & samples, uint16_t bitsPerSample, uint16_t formatCode)
{
    if (formatCode == FormatCode::LinearPCM)
    {
        WriteBulkLinearPCM (out, samples, bitsPerSample);
    }
    else if (formatCode == FormatCode::FloatPCM)
    {
        WriteBulkFloatPCM (out, samples, bitsPerSample);
    }
    else assert (false);
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
                const std::byte * bytes = reinterpret_cast<const std::byte *> (&current);
                buffer [i]   = bytes[0];
                buffer [i+1] = bytes[1];
                buffer [i+2] = bytes[2];
                buffer [i+3] = bytes[3];
            }
            break;
        case 64:
            for (size_t n = 0; n < samplesToWrite; ++n)
            {
                const auto current = static_cast<double> (samples [n+readPosition]);
                const auto i {n * bytesPerSample};
                const std::byte * bytes = reinterpret_cast<const std::byte *> (&current);
                buffer [i]   = bytes[0];
                buffer [i+1] = bytes[1];
                buffer [i+2] = bytes[2];
                buffer [i+3] = bytes[3];
                buffer [i+4] = bytes[4];
                buffer [i+5] = bytes[5];
                buffer [i+6] = bytes[6];
                buffer [i+7] = bytes[7];
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
