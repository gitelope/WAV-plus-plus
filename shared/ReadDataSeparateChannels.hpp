
#ifndef wavplusplus_ReadDataSeparateChannels_hpp
#define wavplusplus_ReadDataSeparateChannels_hpp

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
void ReadDataSeparateChannels (FILE * in,
                               std::vector <std::vector<T>> & destination,
                               Info const & info, DataChunk const & data,
                               size_t requestedBufferSizeInBytes);

template <FloatOrDouble T>
void ReadSeparateChannels_FromBuffer_LinearPCM (std::vector<std::byte> const & buffer,
                                                std::vector<std::vector<T>> & destination,
                                                Info const & info,
                                                size_t framesToRead, size_t writePosition);

template <FloatOrDouble T>
void ReadSeparateChannels_FromBuffer_FloatPCM (std::vector<std::byte> const & buffer,
                                               std::vector<std::vector<T>> & destination,
                                               Info const & info,
                                               size_t framesToRead, size_t writePosition);

//////////////////////////////////////////////////////////////////////////////s

template <FloatOrDouble T>
void ReadDataSeparateChannels (FILE * in,
                               std::vector <std::vector<T>> & destination,
                               Info const & info, DataChunk const & data,
                               size_t requestedBufferSizeInBytes)
{
    // Advance to the beginning of the data portion of the data chunk:
    fseek (in, data.offset, SEEK_SET);
    if (ferror (in)) throw ReadError ("Failed to advance to data portion of data chunk.");

    destination.resize (info.channels, std::vector<T> (info.frames));

    const size_t frameSizeInBytes {static_cast<size_t> (info.channels * info.containerSizeInBits / 8)};
    const size_t totalBytesToRead {info.frames * frameSizeInBytes};

    // Determine buffer size from requested size, frame (a.k.a. block) size, and number of samples:
    // 1. Buffer must be able to hold an entire frame's worth of bytes.
    size_t bufferSizeInBytes = std::max (requestedBufferSizeInBytes, frameSizeInBytes);
    // 2. Buffer size must correspond to a whole number of frames.
    bufferSizeInBytes = (bufferSizeInBytes / frameSizeInBytes) * frameSizeInBytes;
    // 3. Buffer size must not exceed the total number of bytes to read.
    bufferSizeInBytes = std::min (bufferSizeInBytes, totalBytesToRead);

    std::vector<std::byte> buffer (bufferSizeInBytes);

    size_t framesRemaining {static_cast<size_t> (info.frames)};
    size_t writePosition {0};

    while (framesRemaining)
    {
        // Read in bytes:
        const auto bytesToRead {std::min (framesRemaining * frameSizeInBytes, buffer.size())};
        if (fread (buffer.data(), 1, bytesToRead, in) != bytesToRead)
        {
            throw ReadError ("Failed to read buffer from file.");
        }

        // Decode into samples:
        const auto framesToRead {bytesToRead / frameSizeInBytes};
        if      (info.isLinearPCM) ReadSeparateChannels_FromBuffer_LinearPCM (buffer, destination, info, framesToRead, writePosition);
        else if (info.isFloatPCM)  ReadSeparateChannels_FromBuffer_FloatPCM  (buffer, destination, info, framesToRead, writePosition);
        else assert (false);

        writePosition += framesToRead;
        framesRemaining -= framesToRead;
    }
}

template <FloatOrDouble T>
void ReadSeparateChannels_FromBuffer_LinearPCM (std::vector<std::byte> const & buffer,
                                                std::vector<std::vector<T>> & destination,
                                                Info const & info,
                                                size_t framesToRead, size_t writePosition)
{
    // Normalize integers to [-1.0, 1.0) by dividing by 2^(bits-1).
    // Example:
    // 16-bit range: [-32768, 32767], divided by 32768: [-1.0, 0.9999694824]
    // Note: 8-bit also has an offset that is applied first.

    size_t pos {0};
    switch (info.containerSizeInBits)
    {
        case 8:
            for (size_t frame = writePosition; frame < writePosition + framesToRead; ++frame)
            {
                for (size_t channel = 0; channel < info.channels; ++channel)
                {
                    destination [channel][frame] = (uint8 (buffer, pos) - uint8Offset) / uint8MaxAbs;
                }
            }
            break;

        case 16:
            for (size_t frame = writePosition; frame < writePosition + framesToRead; ++frame)
            {
                for (size_t channel = 0; channel < info.channels; ++channel)
                {
                    destination [channel][frame] = int16_le (buffer, pos) / int16MaxAbs;
                }
            }
            break;

        case 24:
            for (size_t frame = writePosition; frame < writePosition + framesToRead; ++frame)
            {
                for (size_t channel = 0; channel < info.channels; ++channel)
                {
                    destination [channel][frame] = int24_le (buffer, pos) / int24MaxAbs;
                }
            }
            break;

        case 32:
            for (size_t frame = writePosition; frame < writePosition + framesToRead; ++frame)
            {
                for (size_t channel = 0; channel < info.channels; ++channel)
                {
                    destination [channel][frame] = int32_le (buffer, pos) / int32MaxAbs;
                }
            }
            break;
    }
}

template <FloatOrDouble T>
void ReadSeparateChannels_FromBuffer_FloatPCM (std::vector<std::byte> const & buffer,
                                               std::vector<std::vector<T>> & destination,
                                               Info const & info,
                                               size_t framesToRead, size_t writePosition)
{
    assert (info.containerSizeInBits == info.bitDepth);

    size_t pos {0};
    switch (info.containerSizeInBits)
    {
        case 32:
            for (size_t frame = writePosition; frame < writePosition + framesToRead; ++frame)
            {
                for (size_t channel = 0; channel < info.channels; ++channel)
                {
                    destination [channel][frame] = float32_le (buffer, pos);
                }
            }
            break;

        case 64:
            for (size_t frame = writePosition; frame < writePosition + framesToRead; ++frame)
            {
                for (size_t channel = 0; channel < info.channels; ++channel)
                {
                    destination [channel][frame] = float64_le (buffer, pos);
                }
            }
            break;
    }
}

}

#endif

