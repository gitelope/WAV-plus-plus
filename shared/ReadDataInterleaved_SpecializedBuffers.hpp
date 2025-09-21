
#ifndef wavplusplus_ReadDataInterleaved_SpecializedBuffers_hpp
#define wavplusplus_ReadDataInterleaved_SpecializedBuffers_hpp

#include "Formats.hpp"
#include "Info.hpp"
#include "ChunkInfo.hpp"
#include "ReadingBytes.hpp"
#include "DequantizationConstants.hpp"
#include "BufferSizing.hpp"
#include "int24.hpp"
#include <cassert>
#include <vector>
#include <cstdio>
#include <bit>

namespace wav
{

template <FloatOrDouble T>
void ReadDataInterleaved (FILE * in,
                          std::vector<T> & destination,
                          Info const & info, DataChunk const & data,
                          size_t requestedBufferSizeInBytes);

template <FloatOrDouble T>
void ReadInterleaved_FloatPCM_NoConversion (FILE * in, std::vector<T> & destination);

template <FloatOrDouble T>
void ReadInterleaved_FloatPCM_WithConversion (FILE * in, std::vector<T> & destination, uint16_t bitsPerSample, size_t readBufferSizeInBytes, size_t ioBufferSizeInBytes);

template <FloatOrDouble T>
void ReadInterleaved_LPCMU8 (FILE * in, std::vector<T> & destination, size_t readBufferSizeInBytes, size_t ioBufferSizeInBytes);

template <FloatOrDouble T>
void ReadInterleaved_LPCM16 (FILE * in, std::vector<T> & destination, size_t readBufferSizeInBytes, size_t ioBufferSizeInBytes);

template <FloatOrDouble T>
void ReadInterleaved_LPCM24 (FILE * in, std::vector<T> & destination, size_t readBufferSizeInBytes, size_t ioBufferSizeInBytes);

template <FloatOrDouble T>
void ReadInterleaved_LPCM32 (FILE * in, std::vector<T> & destination, size_t readBufferSizeInBytes, size_t ioBufferSizeInBytes);

////////////////////////////////////////////////////////////////////////////////////////

template <FloatOrDouble T>
void ReadDataInterleaved (FILE * in,
                          std::vector<T> & destination,
                          Info const & info,
                          DataChunk const & data,
                          size_t requestedBufferSizeInBytes)
{
    constexpr size_t ioBufferSizeMultiplier {4};

    // Advance to the beginning of the data portion of the data chunk:
    if (fsetpos (in, &data.offset) != 0)
    {
        throw ReadError ("Failed to advance to data portion of data chunk.");
    }

    const size_t numSamples {info.frames * info.channels};
    assert (numSamples > 0);

    destination.resize (numSamples);

    const auto readBufferSize {bufferSizeInBytes (requestedBufferSizeInBytes, numSamples, info.bitDepth)};
    const auto ioBufferSize {ioBufferSizeInBytes (numSamples, info.bitDepth, readBufferSize, ioBufferSizeMultiplier)};

    if (info.isLinearPCM)
    {
        switch (info.containerSizeInBits)
        {
            case 8:  ReadInterleaved_LPCMU8 (in, destination, readBufferSize, ioBufferSize); break;
            case 16: ReadInterleaved_LPCM16 (in, destination, readBufferSize, ioBufferSize); break;
            case 24: ReadInterleaved_LPCM24 (in, destination, readBufferSize, ioBufferSize); break;
            case 32: ReadInterleaved_LPCM32 (in, destination, readBufferSize, ioBufferSize); break;
        }
    }
    else if (info.isFloatPCM)
    {
        if ((std::is_same_v<T, float> && info.bitDepth == 32) || (std::is_same_v<T, double> && info.bitDepth == 64))
        {
            ReadInterleaved_FloatPCM_NoConversion (in, destination);
        }
        else
        {
            ReadInterleaved_FloatPCM_WithConversion (in, destination, info.bitDepth, readBufferSize, ioBufferSize);
        }
    }
}

template <FloatOrDouble T>
void ReadInterleaved_FloatPCM_NoConversion (FILE * in, std::vector<T> & destination)
{
    // Input bit depth same as output: float->float or double->double.
    // No need for any conversion. Read data straight from file to destination.
    // No read buffer. No setting internal I/O buffer size.
    if (fread (destination.data(), sizeof (T), destination.size(), in) != destination.size())
    {
        throw ReadError ("Failed to read buffer from file.");
    }
}

template <FloatOrDouble T>
void ReadInterleaved_FloatPCM_WithConversion (FILE * in, std::vector<T> & destination, uint16_t bitsPerSample, size_t readBufferSizeInBytes, size_t ioBufferSizeInBytes)
{
    // Input is different bit depth than output: float->double or double->float.
    // Read into a buffer of the input type, then convert to output type.

    SetInternalIOBufferSize<ReadError> (in, ioBufferSizeInBytes);

    const size_t inputSize {(bitsPerSample == 32) ? sizeof (float) : sizeof (double)};
    const size_t bufferSize {readBufferSizeInBytes / inputSize};

    size_t samplesRemaining {destination.size()};
    size_t readPosition {0};

    if (bitsPerSample == 32)
    {
        // Input is float, output should be double.
        std::vector<float> buffer (bufferSize);
        while (samplesRemaining)
        {
            const size_t samplesToRead {std::min (samplesRemaining, buffer.size())};
            const size_t bytesToRead {samplesToRead * inputSize};
            if (fread (buffer.data(), 1, bytesToRead, in) != bytesToRead)
            {
                throw ReadError ("Failed to read buffer from file.");
            }

            // Compile-time byteswap if needed (C++20 + C++23)
            if constexpr (std::endian::native == std::endian::big)
            {
                for (size_t i = 0; i < samplesToRead; ++i)
                {
                    buffer[i] = std::bit_cast<float> (std::byteswap (std::bit_cast<uint32_t> (buffer[i])));
                }
            }

            for (size_t i=0; i < samplesToRead; ++i)
            {
                destination [readPosition + i] = static_cast<T> (buffer [i]);
            }
            readPosition += samplesToRead;
            samplesRemaining -= samplesToRead;
        }
    }
    else
    {
        // Input is double, output should be float.
        std::vector<double> buffer (bufferSize);
        while (samplesRemaining)
        {
            const size_t samplesToRead {std::min (samplesRemaining, buffer.size())};
            const size_t bytesToRead {samplesToRead * inputSize};
            if (fread (buffer.data(), 1, bytesToRead, in) != bytesToRead)
            {
                throw ReadError ("Failed to read buffer from file.");
            }

            // Compile-time byteswap if needed (C++20 + C++23)
            if constexpr (std::endian::native == std::endian::big)
            {
                for (size_t i = 0; i < samplesToRead; ++i)
                {
                    buffer[i] = std::bit_cast<double>(std::byteswap(std::bit_cast<uint64_t>(buffer[i])));
                }
            }

            for (size_t i=0; i < samplesToRead; ++i)
            {
                destination [readPosition + i] = static_cast<T> (buffer [i]);
            }
            readPosition += samplesToRead;
            samplesRemaining -= samplesToRead;
        }
    }
}

template <FloatOrDouble T>
void ReadInterleaved_LPCMU8 (FILE * in, std::vector<T> & destination, size_t readBufferSizeInBytes, size_t ioBufferSizeInBytes)
{
    constexpr size_t bytesPerSample {1};
    const auto max {static_cast<T> (uint8MaxAbs)};
    const auto offset {static_cast<T> (uint8Offset)};

    std::vector<uint8_t> buffer (readBufferSizeInBytes);
    SetInternalIOBufferSize<ReadError> (in, ioBufferSizeInBytes);

    size_t samplesRemaining {destination.size()};
    size_t readPosition {0};

    while (samplesRemaining)
    {
        const auto samplesToRead {std::min (samplesRemaining, buffer.size())};
        if (fread (buffer.data(), bytesPerSample, samplesToRead, in) != samplesToRead)
        {
            throw ReadError ("Failed to read buffer from file.");
        }
        for (size_t i=0; i < samplesToRead; ++i)
        {
            destination [readPosition + i] = (buffer [i] - offset) / max;
        }
        readPosition += samplesToRead;
        samplesRemaining -= samplesToRead;
    }
}

template <FloatOrDouble T>
void ReadInterleaved_LPCM16 (FILE * in, std::vector<T> & destination, size_t readBufferSizeInBytes, size_t ioBufferSizeInBytes)
{
    constexpr size_t bytesPerSample {2};
    const auto max {static_cast<T> (int16MaxAbs)};

    std::vector<int16_t> buffer (readBufferSizeInBytes / bytesPerSample);
    SetInternalIOBufferSize<ReadError> (in, ioBufferSizeInBytes);

    size_t samplesRemaining {destination.size()};
    size_t readPosition {0};

    while (samplesRemaining)
    {
        const auto samplesToRead {std::min (samplesRemaining, buffer.size())};
        if (fread (buffer.data(), bytesPerSample, samplesToRead, in) != samplesToRead)
        {
            throw ReadError ("Failed to read buffer from file.");
        }

        // Byteswap if needed.
        if constexpr (std::endian::native == std::endian::big)
        {
            for (size_t i = 0; i < samplesToRead; ++i)
            {
                buffer[i] = std::byteswap (buffer[i]);
            }
        }

        for (size_t i=0; i < samplesToRead; ++i)
        {
            destination [readPosition + i] = buffer [i] / max;
        }
        readPosition += samplesToRead;
        samplesRemaining -= samplesToRead;
    }
}

template <FloatOrDouble T>
void ReadInterleaved_LPCM24 (FILE * in, std::vector<T> & destination, size_t readBufferSizeInBytes, size_t ioBufferSizeInBytes)
{
    constexpr size_t bytesPerSample {3};
    const auto max {static_cast<T> (int24MaxAbs)};

    std::vector<int24> buffer (readBufferSizeInBytes / bytesPerSample);
    SetInternalIOBufferSize<ReadError> (in, ioBufferSizeInBytes);

    size_t samplesRemaining {destination.size()};
    size_t readPosition {0};

    while (samplesRemaining)
    {
        const auto samplesToRead {std::min (samplesRemaining, buffer.size())};
        if (fread (buffer.data(), bytesPerSample, samplesToRead, in) != samplesToRead)
        {
            throw ReadError ("Failed to read buffer from file.");
        }

        // Byteswap if needed.
        if constexpr (std::endian::native == std::endian::big)
        {
            for (size_t i = 0; i < samplesToRead; ++i)
            {
                std::swap (buffer[i].bytes[0], buffer[i].bytes[2]);
            }
        }

        for (size_t i=0; i < samplesToRead; ++i)
        {
            destination [readPosition + i] = buffer[i] / max;
        }
        readPosition += samplesToRead;
        samplesRemaining -= samplesToRead;
    }
}

template <FloatOrDouble T>
void ReadInterleaved_LPCM32 (FILE * in, std::vector<T> & destination, size_t readBufferSizeInBytes, size_t ioBufferSizeInBytes)
{
    constexpr size_t bytesPerSample {4};
    const auto max {static_cast<T> (int32MaxAbs)};

    std::vector<int32_t> buffer (readBufferSizeInBytes / bytesPerSample);
    SetInternalIOBufferSize<ReadError> (in, ioBufferSizeInBytes);

    size_t samplesRemaining {destination.size()};
    size_t readPosition {0};

    while (samplesRemaining)
    {
        const auto samplesToRead {std::min (samplesRemaining, buffer.size())};
        if (fread (buffer.data(), bytesPerSample, samplesToRead, in) != samplesToRead)
        {
            throw ReadError ("Failed to read buffer from file.");
        }

        // Byteswap if needed.
        if constexpr (std::endian::native == std::endian::big)
        {
            for (size_t i = 0; i < samplesToRead; ++i)
            {
                buffer[i] = std::byteswap (buffer[i]);
            }
        }

        for (size_t i=0; i < samplesToRead; ++i)
        {
            destination [readPosition + i] = buffer [i] / max;
        }
        readPosition += samplesToRead;
        samplesRemaining -= samplesToRead;
    }
}

} // namespace wav

#endif
