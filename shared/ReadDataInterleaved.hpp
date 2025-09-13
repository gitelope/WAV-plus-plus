
#ifndef wavplusplus_ReadDataInterleaved_hpp
#define wavplusplus_ReadDataInterleaved_hpp

#include "Formats.hpp"
#include "Info.hpp"
#include "ChunkInfo.hpp"
#include "ReadingBytes.hpp"
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
void ReadInterleaved_FromBuffer_LinearPCM (std::vector<std::byte> const & buffer,
                                           std::vector<T> & destination,
                                           Info const & info,
                                           size_t samplesToRead, size_t writePosition);

template <FloatOrDouble T>
void ReadInterleaved_FromBuffer_FloatPCM (std::vector<std::byte> const & buffer,
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
    fseek (in, data.offset, SEEK_SET);
    if (ferror (in)) throw ReadError ("Failed to advance to data portion of data chunk.");

    const size_t numSamples {static_cast<size_t>(info.frames) * info.channels};
    assert (numSamples > 0);

    destination.resize (numSamples);

    const size_t sampleSizeInBytes {static_cast<size_t> (info.containerSizeInBits / 8)};
    const size_t totalBytesToRead {numSamples * sampleSizeInBytes};

    // Determine buffer size from requested size, sample size, and number of samples:
    // 1. Buffer must be able to hold an entire sample's worth of bytes.
    size_t bufferSizeInBytes = std::max (requestedBufferSizeInBytes, sampleSizeInBytes);
    // 2. Buffer size must correspond to a whole number of samples.
    bufferSizeInBytes = (bufferSizeInBytes / sampleSizeInBytes) * sampleSizeInBytes;
    // 3. Buffer size must not exceed the total number of bytes to read.
    bufferSizeInBytes = std::min (bufferSizeInBytes, totalBytesToRead);

    std::vector<std::byte> buffer (bufferSizeInBytes);

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
        if      (info.isLinearPCM) ReadInterleaved_FromBuffer_LinearPCM (buffer, destination, info, samplesToRead, writePosition);
        else if (info.isFloatPCM)  ReadInterleaved_FromBuffer_FloatPCM  (buffer, destination, info, samplesToRead, writePosition);
        else assert (false);

        writePosition += samplesToRead;
        samplesRemaining -= samplesToRead;
    }
}

template <FloatOrDouble T>
void ReadInterleaved_FromBuffer_LinearPCM (std::vector<std::byte> const & buffer,
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
            for (size_t n = writePosition; n < samplesToRead + writePosition; ++n)
            {
                destination [n] = (uint8 (buffer, pos) - uint8Offset) / uint8MaxAbs;
            }
            break;

        case 16:
            for (size_t n = writePosition; n < samplesToRead + writePosition; ++n)
            {
                destination [n] = int16_le (buffer, pos) / int16MaxAbs;
            }
            break;

        case 24:
            for (size_t n = writePosition; n < samplesToRead + writePosition; ++n)
                destination [n] = int24_le (buffer, pos) / int24MaxAbs;
            break;

        case 32:
            for (size_t n = writePosition; n < samplesToRead + writePosition; ++n)
                destination [n] = int32_le (buffer, pos) / int32MaxAbs;
            break;
    }
}

template <FloatOrDouble T>
void ReadInterleaved_FromBuffer_FloatPCM (std::vector<std::byte> const & buffer,
                                          std::vector<T> & destination,
                                          Info const & info,
                                          size_t samplesToRead, size_t writePosition)
{
    assert (info.containerSizeInBits == info.bitDepth);

    size_t pos {0};
    switch (info.containerSizeInBits)
    {
        case 32:
            for (size_t n = writePosition; n < samplesToRead + writePosition; ++n)
                destination [n] = float32_le (buffer, pos);
            break;

        case 64:
            for (size_t n = writePosition; n < samplesToRead + writePosition; ++n)
                destination [n] = float64_le (buffer, pos);
            break;
    }
}

} // namespace wav

#endif
