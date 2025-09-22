
#ifndef wavplusplus_ReadDataInterleaved_ByteBuffer_Memcpy_hpp
#define wavplusplus_ReadDataInterleaved_ByteBuffer_Memcpy_hpp

#include "Formats.hpp"
#include "Info.hpp"
#include "ChunkInfo.hpp"
#include "BufferSizing.hpp"
#include "int24.hpp"
#include <cassert>
#include <vector>
#include <cstdio>

namespace wav
{

template <FloatOrDouble T>
void ReadDataInterleaved (FILE * in,
                          std::vector<T> & destination,
                          Info const & info, DataChunk const & data,
                          size_t requestedBufferSizeInBytes);

template <FloatOrDouble T>
void ReadLinearPCM (std::vector<std::byte> const & buffer,
                    std::vector<T> & destination,
                    Info const & info,
                    size_t samplesToRead, size_t writePosition);

template <FloatOrDouble T>
void ReadFloatPCM (std::vector<std::byte> const & buffer,
                   std::vector<T> & destination,
                   Info const & info,
                   size_t samplesToRead, size_t writePosition);

///////////////////////////////////////////////////////////////////////

template <FloatOrDouble T>
void ReadDataInterleaved (FILE * in,
                          std::vector<T> & destination,
                          Info const & info,
                          DataChunk const & data,
                          size_t requestedBufferSizeInBytes)
{
    // Advance to the beginning of the data portion of the data chunk:
    if (fsetpos (in, &data.offset) != 0)
    {
        throw ReadError ("Failed to advance to data portion of data chunk.");
    }

    const size_t numSamples {info.frames * info.channels};
    assert (numSamples > 0);

    destination.resize (numSamples);

    const auto readBufferSize {bufferSizeInBytes (requestedBufferSizeInBytes, numSamples, info.bitDepth)};
    std::vector<std::byte> buffer (readBufferSize);

    const size_t sampleSizeInBytes {info.containerSizeInBits / 8U};

    size_t samplesRemaining {numSamples};
    size_t writePosition {0};

    while (samplesRemaining)
    {
        // Read in bytes:
        const auto bytesToRead {std::min (samplesRemaining * sampleSizeInBytes, buffer.size())};
        if (fread (buffer.data(), 1, bytesToRead, in) != bytesToRead)
        {
            throw ReadError ("Failed to read buffer from file.");
        }

        // Decode into samples:
        const auto samplesToRead {bytesToRead / sampleSizeInBytes};
        if      (info.isLinearPCM) ReadLinearPCM (buffer, destination, info, samplesToRead, writePosition);
        else if (info.isFloatPCM)  ReadFloatPCM  (buffer, destination, info, samplesToRead, writePosition);
        else assert (false);

        writePosition += samplesToRead;
        samplesRemaining -= samplesToRead;
    }
}

template <FloatOrDouble T>
void ReadLinearPCM (std::vector<std::byte> const & buffer,
                    std::vector<T> & destination,
                    Info const & info,
                    size_t samplesToRead, size_t writePosition)
{
    // Normalize integers to [-1.0, 1.0) by dividing by 2^(bits-1).
    // Example:
    // 16-bit range: [-32768, 32767], divided by 32768: [-1.0, 0.9999694824]
    // Note: 8-bit also has an offset that is applied first.

    assert (info.bitDepth == info.containerSizeInBits);

    switch (info.bitDepth)
    {
        case 8:
        {
            if (samplesToRead > buffer.size())
            {
                throw ReadError ("Not enough bytes to read uint8_t samples.");
            }
            for (size_t i = 0; i < samplesToRead; ++i)
            {
                destination[writePosition + i] = static_cast<T> ((static_cast<uint8_t> (buffer[i]) - uint8Offset) / uint8MaxAbs);
            }
            break;
        }
        case 16:
        {
            const size_t bytesToRead {samplesToRead * 2};
            if (bytesToRead > buffer.size())
            {
                throw ReadError ("Not enough bytes to read int16_t samples.");
            }
            std::vector<int16_t> temp (samplesToRead);
            std::memcpy (temp.data(), buffer.data(), bytesToRead);

            for (size_t i = 0; i < samplesToRead; ++i)
            {
                destination[writePosition + i] = static_cast<T> (temp[i] / int16MaxAbs);
            }
            break;
        }
        case 24:
        {
            const size_t bytesToRead {samplesToRead * 3};
            if (bytesToRead > buffer.size())
            {
                throw ReadError ("Not enough bytes to read int24_t samples.");
            }
            std::vector<int24> temp (samplesToRead);
            std::memcpy (temp.data(), buffer.data(), bytesToRead);

            for (size_t i = 0; i < samplesToRead; ++i)
            {
                destination[writePosition + i] = static_cast<T> (static_cast<int32_t> (temp[i]) / int24MaxAbs);
            }
            break;
        }

        case 32:
        {
            const size_t bytesToRead {samplesToRead * 4};
            if (bytesToRead > buffer.size())
            {
                throw ReadError ("Not enough bytes to read int32_t samples.");
            }
            std::vector<int32_t> temp (samplesToRead);
            std::memcpy (temp.data(), buffer.data(), bytesToRead);

            for (size_t i = 0; i < samplesToRead; ++i)
            {
                destination[writePosition + i] = static_cast<T>(temp[i] / int32MaxAbs);
            }
            break;
        }
    }
}

template <FloatOrDouble T>
void ReadFloatPCM (std::vector<std::byte> const & buffer,
                   std::vector<T> & destination,
                   Info const & info,
                   size_t samplesToRead, size_t writePosition)
{
    assert (info.containerSizeInBits == info.bitDepth);

    switch (info.bitDepth)
    {
        case 32:
        {
            const size_t bytesToRead {samplesToRead * 4};
            if (bytesToRead > buffer.size())
            {
                throw ReadError ("Not enough bytes to read float32_t samples.");
            }
            std::vector<float> temp (samplesToRead);
            std::memcpy (temp.data(), buffer.data(), bytesToRead);

            for (size_t i = 0; i < samplesToRead; ++i)
            {
                destination[writePosition + i] = static_cast<T> (temp[i]);
            }
            break;
        }
        case 64:
        {
            const size_t bytesToRead {samplesToRead * 8};
            if (bytesToRead > buffer.size())
            {
                throw ReadError ("Not enough bytes to read float64_t samples.");
            }
            std::vector<double> temp (samplesToRead);
            std::memcpy (temp.data(), buffer.data(), bytesToRead);

            for (size_t i = 0; i < samplesToRead; ++i)
            {
                destination[writePosition + i] = static_cast<T> (temp[i]);
            }
            break;
        }
    }
}

}

#endif
