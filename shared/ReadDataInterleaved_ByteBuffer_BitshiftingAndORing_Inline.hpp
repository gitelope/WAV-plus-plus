
#ifndef wavplusplus_ReadDataInterleaved_ByteBuffer_BitshiftingAndORing_Inline_hpp
#define wavplusplus_ReadDataInterleaved_ByteBuffer_BitshiftingAndORing_Inline_hpp

#include "Formats.hpp"
#include "Info.hpp"
#include "ChunkInfo.hpp"
#include "ReadingBytes.hpp"
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


////////////////////////////////////////////////////////////////////////////////////////

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

    size_t pos {0};
    switch (info.containerSizeInBits)
    {
        case 8:
        {
            if (samplesToRead > buffer.size())
            {
                throw ReadError ("Not enough bytes to read uint8_t samples.");
            }
            for (size_t n = writePosition; n < samplesToRead + writePosition; ++n)
            {
                destination [n] = static_cast<T> ((static_cast<uint8_t> (buffer[pos]) - uint8Offset) / uint8MaxAbs);
                ++pos;
            }
            break;
        }

        case 16:
        {
            if (samplesToRead * 2 > buffer.size())
            {
                throw ReadError ("Not enough bytes to read int16_t samples.");
            }
            for (size_t n = writePosition; n < samplesToRead + writePosition; ++n)
            {
                const auto sample = static_cast<int16_t>
                (
                 static_cast<int16_t> (buffer [pos])      |
                 static_cast<int16_t> (buffer [pos+1]) << 8
                 );
                destination [n] = static_cast<T> (sample / int16MaxAbs);
                pos += 2;
            }
            break;
        }

        case 24:
        {
            if (samplesToRead * 3 > buffer.size())
            {
                throw ReadError ("Not enough bytes to read int24_t samples.");
            }
            for (size_t n = writePosition; n < samplesToRead + writePosition; ++n)
            {
                auto sample =
                static_cast<int32_t> (buffer [pos])        |
                static_cast<int32_t> (buffer [pos+1]) << 8 |
                static_cast<int32_t> (buffer [pos+2]) << 16;

                // Sign extension for 24-bit
                if (sample & 0x800000)
                {
                    sample |= 0xFF000000;
                }

                destination [n] = static_cast<T> (sample / int24MaxAbs);
                pos += 3;
            }
            break;
        }

        case 32:
        {
            if (samplesToRead * 4 > buffer.size())
            {
                throw ReadError ("Not enough bytes to read int32_t samples.");
            }
            for (size_t n = writePosition; n < samplesToRead + writePosition; ++n)
            {
                const auto sample = static_cast<int32_t>
                (
                 static_cast<int32_t> (buffer [pos])         |
                 static_cast<int32_t> (buffer [pos+1]) << 8  |
                 static_cast<int32_t> (buffer [pos+2]) << 16 |
                 static_cast<int32_t> (buffer [pos+3]) << 24
                 );
                destination [n] = static_cast<T> (sample / int32MaxAbs);
                pos += 4;
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

    size_t pos {0};
    switch (info.bitDepth)
    {
        case 32:
        {
            if (samplesToRead * 4 > buffer.size())
            {
                throw ReadError ("Not enough bytes to read float32_t samples.");
            }
            for (size_t n = writePosition; n < samplesToRead + writePosition; ++n)
            {
                const auto sample = static_cast<float>
                (
                 static_cast<uint32_t> (buffer [pos])         |
                 static_cast<uint32_t> (buffer [pos+1]) << 8  |
                 static_cast<uint32_t> (buffer [pos+2]) << 16 |
                 static_cast<uint32_t> (buffer [pos+3]) << 24
                 );
                destination [n] = static_cast<T> (sample);
                pos += 4;
            }
            break;
        }

        case 64:
        {
            if (samplesToRead * 8 > buffer.size())
            {
                throw ReadError ("Not enough bytes to read float64_t samples.");
            }
            for (size_t n = writePosition; n < samplesToRead + writePosition; ++n)
            {
                const auto sample = static_cast<double>
                (
                 static_cast<uint64_t> (buffer [pos])         |
                 static_cast<uint64_t> (buffer [pos+1]) << 8  |
                 static_cast<uint64_t> (buffer [pos+2]) << 16 |
                 static_cast<uint64_t> (buffer [pos+3]) << 24 |
                 static_cast<uint64_t> (buffer [pos+4]) << 32 |
                 static_cast<uint64_t> (buffer [pos+5]) << 40 |
                 static_cast<uint64_t> (buffer [pos+6]) << 48 |
                 static_cast<uint64_t> (buffer [pos+7]) << 56
                 );
                destination [n] = static_cast<T> (sample);
                pos += 8;
            }
            break;
        }
    }
}

}

#endif

