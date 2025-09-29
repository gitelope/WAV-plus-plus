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
                          Info const & info,
                          DataChunk const & data,
                          size_t requestedBufferSizeInBytes);

template <FloatOrDouble T>
void ReadLPCM8 (FILE * in, std::vector<std::byte> & buffer, std::vector<T> & destination);

template <FloatOrDouble T>
void ReadLPCM16 (FILE * in, std::vector<std::byte> & buffer, std::vector<T> & destination);

template <FloatOrDouble T>
void ReadLPCM24 (FILE * in, std::vector<std::byte> & buffer, std::vector<T> & destination);

template <FloatOrDouble T>
void ReadLPCM32 (FILE * in, std::vector<std::byte> & buffer, std::vector<T> & destination);

template <FloatOrDouble T>
void ReadFloatPCM_NoConversion (FILE * in, std::vector<std::byte> & buffer, std::vector<T> & destination);

void ReadFloatToDouble (FILE * in, std::vector<std::byte> & buffer, std::vector<double> & destination);

void ReadDoubleToFloat (FILE * in, std::vector<std::byte> & buffer, std::vector<float> & destination);

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

    if (info.isLinearPCM)
    {
        switch (info.bitDepth)
        {
            case 8:  ReadLPCM8  (in, buffer, destination); break;
            case 16: ReadLPCM16 (in, buffer, destination); break;
            case 24: ReadLPCM24 (in, buffer, destination); break;
            case 32: ReadLPCM32 (in, buffer, destination); break;
            default: throw ReadError ("Unsupported Linear PCM bit depth.");
        }
    }
    else if (info.isFloatPCM)
    {
        switch (info.bitDepth)
        {
            case 32: 
                if constexpr (std::is_same_v<T, double>)
                    ReadFloatToDouble (in, buffer, destination);
                else
                    ReadFloatPCM_NoConversion (in, buffer, destination);
                break;
            case 64: 
                if constexpr (std::is_same_v<T, float>)
                    ReadDoubleToFloat (in, buffer, destination);
                else
                    ReadFloatPCM_NoConversion (in, buffer, destination);
                break;
            default: throw ReadError ("Unsupported Float PCM bit depth.");
        }
    }
    else
    {
        throw ReadError ("Unsupported audio format.");
    }
}

template <FloatOrDouble T>
void ReadLPCM8 (FILE * in, std::vector<std::byte> & buffer, std::vector<T> & destination)
{
    constexpr size_t sampleSizeInBytes {1};

    size_t samplesRemaining {destination.size()};
    size_t writePosition {0};

    while (samplesRemaining)
    {
        // Read in bytes.
        const auto bytesToRead {std::min (samplesRemaining * sampleSizeInBytes, buffer.size())};
        if (fread (buffer.data(), 1, bytesToRead, in) != bytesToRead)
        {
            throw ReadError ("Failed to read buffer from file.");
        }

        // Convert to normalized floating-point and write to destination.
        for (size_t i = 0; i < bytesToRead; ++i)
        {
            destination[writePosition + i] = static_cast<T> ((static_cast<uint8_t> (buffer[i]) - uint8Offset) / uint8MaxAbs);
        }

        writePosition += bytesToRead;
        samplesRemaining -= bytesToRead;
    }
}

template <FloatOrDouble T>
void ReadLPCM16 (FILE * in, std::vector<std::byte> & buffer, std::vector<T> & destination)
{
    constexpr size_t sampleSizeInBytes {2};

    std::vector<int16_t> integers (buffer.size() / sampleSizeInBytes);

    size_t samplesRemaining {destination.size()};
    size_t writePosition {0};

    while (samplesRemaining)
    {
        // Read in bytes.
        const auto bytesToRead {std::min (samplesRemaining * sampleSizeInBytes, buffer.size())};
        if (fread (buffer.data(), 1, bytesToRead, in) != bytesToRead)
        {
            throw ReadError ("Failed to read buffer from file.");
        }

        // Copy as 16-bit ints.
        std::memcpy (integers.data(), buffer.data(), bytesToRead);

        // Convert to normalized floating-point.
        const auto samplesToRead {bytesToRead / sampleSizeInBytes};
        for (size_t i = 0; i < samplesToRead; ++i)
        {
            destination[writePosition + i] = static_cast<T> (integers[i] / int16MaxAbs);
        }

        writePosition += samplesToRead;
        samplesRemaining -= samplesToRead;
    }
}

template <FloatOrDouble T>
void ReadLPCM24 (FILE * in, std::vector<std::byte> & buffer, std::vector<T> & destination)
{
    constexpr size_t sampleSizeInBytes {3};

    std::vector<int24> integers (buffer.size() / sampleSizeInBytes);

    size_t samplesRemaining {destination.size()};
    size_t writePosition {0};

    while (samplesRemaining)
    {
        // Read in bytes.
        const auto bytesToRead {std::min (samplesRemaining * sampleSizeInBytes, buffer.size())};
        if (fread (buffer.data(), 1, bytesToRead, in) != bytesToRead)
        {
            throw ReadError ("Failed to read buffer from file.");
        }

        // Copy as 24-bit ints.
        std::memcpy (integers.data(), buffer.data(), bytesToRead);

        // Convert to normalized floating-point.
        const auto samplesToRead {bytesToRead / sampleSizeInBytes};
        for (size_t i = 0; i < samplesToRead; ++i)
        {
            destination[writePosition + i] = static_cast<T> (static_cast<int32_t> (integers[i]) / int24MaxAbs);
        }

        writePosition += samplesToRead;
        samplesRemaining -= samplesToRead;
    }
}

template <FloatOrDouble T>
void ReadLPCM32 (FILE * in, std::vector<std::byte> & buffer, std::vector<T> & destination)
{
    constexpr size_t sampleSizeInBytes {4};

    std::vector<int32_t> integers (buffer.size() / sampleSizeInBytes);

    size_t samplesRemaining {destination.size()};
    size_t writePosition {0};

    while (samplesRemaining)
    {
        // Read in bytes.
        const auto bytesToRead {std::min (samplesRemaining * sampleSizeInBytes, buffer.size())};
        if (fread (buffer.data(), 1, bytesToRead, in) != bytesToRead)
        {
            throw ReadError ("Failed to read buffer from file.");
        }

        // Copy as 32-bit ints.
        std::memcpy (integers.data(), buffer.data(), bytesToRead);

        // Convert to normalized floating-point.
        const auto samplesToRead {bytesToRead / sampleSizeInBytes};
        for (size_t i = 0; i < samplesToRead; ++i)
        {
            destination[writePosition + i] = static_cast<T> (integers[i] / int32MaxAbs);
        }

        writePosition += samplesToRead;
        samplesRemaining -= samplesToRead;
    }
}

template <FloatOrDouble T>
void ReadFloatPCM_NoConversion (FILE * in, std::vector<std::byte> & buffer, std::vector<T> & destination)
{
    constexpr size_t sampleSizeInBytes {sizeof (T)};
    static_assert ((sampleSizeInBytes == 4 || sampleSizeInBytes == 8) && CHAR_BIT == 8);

    size_t samplesRemaining {destination.size()};
    size_t writePosition = 0;

    while (samplesRemaining)
    {
        // Read in bytes.
        const auto bytesToRead {std::min (samplesRemaining * sampleSizeInBytes, buffer.size())};
        if (fread (buffer.data(), 1, bytesToRead, in) != bytesToRead)
        {
            throw ReadError ("Failed to read buffer from file.");
        }

        // Copy as float or double directly into destination.
        // (Since encoding and decoding format are the same. No further conversion is needed.)
        std::memcpy (&(destination[writePosition]), buffer.data(), bytesToRead);

        const auto samplesToRead {bytesToRead / sampleSizeInBytes};
        writePosition += samplesToRead;
        samplesRemaining -= samplesToRead;
    }
}

void ReadFloatToDouble (FILE * in, std::vector<std::byte> & buffer, std::vector<double> & destination)
{
    constexpr size_t sampleSizeInBytes {4};

    size_t samplesRemaining {destination.size()};
    size_t writePosition = 0;

    std::vector<float> floats (buffer.size() / sampleSizeInBytes);
    static_assert (sizeof (float) == 4 && CHAR_BIT == 8);

    while (samplesRemaining)
    {
        // Read in bytes.
        const auto bytesToRead {std::min (samplesRemaining * sampleSizeInBytes, buffer.size())};
        if (fread (buffer.data(), 1, bytesToRead, in) != bytesToRead)
        {
            throw ReadError ("Failed to read buffer from file.");
        }

        // Copy as floats.
        std::memcpy (floats.data(), buffer.data(), bytesToRead);

        // Convert to double and write to destination.
        const auto samplesToRead {bytesToRead / sampleSizeInBytes};
        for (size_t i = 0; i < samplesToRead; ++i)
        {
            destination[writePosition + i] = static_cast<double> (floats[i]);
        }

        writePosition += samplesToRead;
        samplesRemaining -= samplesToRead;
    }
}

void ReadDoubleToFloat (FILE * in, std::vector<std::byte> & buffer, std::vector<float> & destination)
{
    constexpr size_t sampleSizeInBytes {8};

    size_t samplesRemaining {destination.size()};
    size_t writePosition = 0;

    std::vector<double> doubles (buffer.size() / sampleSizeInBytes);
    static_assert (sizeof (double) == 8 && CHAR_BIT == 8);

    while (samplesRemaining)
    {
        // Read in bytes.
        const auto bytesToRead {std::min (samplesRemaining * sampleSizeInBytes, buffer.size())};
        if (fread (buffer.data(), 1, bytesToRead, in) != bytesToRead)
        {
            throw ReadError("Failed to read buffer from file.");
        }

        // Copy as doubles.
        std::memcpy (doubles.data(), buffer.data(), bytesToRead);

        // Convert to float and write to destination.
        const auto samplesToRead {bytesToRead / sampleSizeInBytes};
        for (size_t i = 0; i < samplesToRead; ++i)
        {
            destination[writePosition + i] = static_cast<float> (doubles[i]);
        }

        writePosition += samplesToRead;
        samplesRemaining -= samplesToRead;
    }
}

} // namespace wav

#endif

