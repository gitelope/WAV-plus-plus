
#ifndef wavplusplus_Read_hpp
#define wavplusplus_Read_hpp

#include "ChunkInfo.hpp"
#include "Info.hpp"
#include "Exceptions.hpp"
#include "DequantizationConstants.hpp"
#include "ReadingBytes.hpp"
#include "ReadDataInterleaved.hpp"

#include <string>
#include <vector>
#include <filesystem>
#include <cstdio>
#include <cassert>

namespace wav
{

// 32kB default (requested) buffer size
inline constexpr size_t defaultRequestedReadBufferSize = 32768;

// Returns pair containing interleaved audio vector and Info.
// Calls in-place Read() overload.
// May throw wav::ReadError.
template <FloatOrDouble T>
std::pair <std::vector<T>, Info>
ReadInterleaved (std::string const & filepath,
                 size_t requestedBufferSize = defaultRequestedReadBufferSize);

// Returns pair containing audio separated into channels and Info.
// Calls in-place Read() overload.
// May throw wav::ReadError.
template <FloatOrDouble T>
std::pair <std::vector<std::vector<T>>, Info>
ReadSeparateChannels (std::string const & filepath,
                      size_t requestedBufferSize = defaultRequestedReadBufferSize);


// In-place read of interleaved audio and Info.
// May throw wav::ReadError.
template <FloatOrDouble T>
void ReadInterleaved (std::string const & filepath,
                      std::vector<T> & destination,
                      Info & info,
                      size_t requestedBufferSize = defaultRequestedReadBufferSize);

// In-place read to separate channels and Info.
// May throw wav::ReadError.
template <FloatOrDouble T>
void ReadSeparateChannels (std::string const & filepath,
                           std::vector<std::vector<T>> & destination,
                           Info & info,
                           size_t requestedBufferSize = defaultRequestedReadBufferSize);

//////////////////////////////////////////////////////////////////

template <FloatOrDouble T>
std::pair <std::vector<T>, Info>
ReadInterleaved (std::string const & filepath, size_t requestedBufferSize)
{
    std::vector<T> data;
    Info info;
    ReadInterleaved (filepath, data, info, requestedBufferSize);
    return {std::move (data), info};
}

// De-interleaved version
template <FloatOrDouble T>
std::pair <std::vector<std::vector<T>>, Info>
ReadSeparateChannels (std::string const & filepath, size_t requestedBufferSize)
{
    std::vector<std::vector<T>> data;
    Info info;
    ReadSeparateChannels (filepath, data, info, requestedBufferSize);
    return {std::move (data), info};
}

template <FloatOrDouble T>
void ReadInterleaved (std::string const & filepath,
                      std::vector<T> & destination,
                      Info & info,
                      size_t requestedBufferSize)
{
    auto in = fileForReading (filepath);
    if (!in) throw ReadError ("Failed to open: " + filepath);

    const long fileLength = std::filesystem::file_size (filepath);
    
    ChunkInfo chunkInfo {in.get()};

    if (chunkInfo.riff.size + 8 != fileLength)
    {
        throw ReadError ("RIFF chunk size + 8 does not equal file size for " + filepath);
    }

    info.ExtractFrom (chunkInfo);
    ReadDataInterleaved (in.get(), destination, info, chunkInfo.data, requestedBufferSize);

    Close (in);
}

template <FloatOrDouble T>
void ReadSeparateChannels (std::string const & filepath,
                           std::vector<std::vector<T>> & destination,
                           Info & info,
                           size_t requestedBufferSize)
{
    auto in = fileForReading (filepath);
    if (!in) throw ReadError ("Failed to open: " + filepath);

    const long fileLength = std::filesystem::file_size (filepath);
    
    ChunkInfo chunkInfo {in.get()};

    if (chunkInfo.riff.size + 8 != fileLength)
    {
        throw ReadError ("RIFF chunk size + 8 does not equal file size for " + filepath);
    }

    info.ExtractFrom (chunkInfo);
    ReadDataSeparateChannels (in.get(), destination, info, chunkInfo.data, requestedBufferSize);
    
    Close (in);
}

template <FloatOrDouble T>
void ReadDataSeparateChannels (FILE * in,
                               std::vector <std::vector<T>> & destination,
                               Info const & info,
                               DataChunk const & data,
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
        if      (info.isLinearPCM) ReadLinearPCM (buffer, destination, info, framesToRead, writePosition);
        else if (info.isFloatPCM)  ReadFloatPCM  (buffer, destination, info, framesToRead, writePosition);
        else assert (false);

        writePosition += framesToRead;
        framesRemaining -= framesToRead;
    }
}

template <FloatOrDouble T>
void ReadLinearPCM (std::vector<std::byte> const & buffer,
                    std::vector<std::vector<T>> & destination,
                    Info const & info,
                    size_t framesToRead,
                    size_t writePosition)
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
void ReadFloatPCM (std::vector<std::byte> const & buffer,
                   std::vector<std::vector<T>> & destination,
                   Info const & info,
                   size_t framesToRead,
                   size_t writePosition)
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

/*
template <typename T>
T powerOfTwo (int power)
{
    // Must be positive. Must not shift past the MSB.
    assert (power > 0 && power < 64);
    return static_cast<T> (static_cast<uint64_t>(1) << power);
}
*/

} // namespace wav

#endif
