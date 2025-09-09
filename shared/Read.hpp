
#ifndef wavplusplus_Read_hpp
#define wavplusplus_Read_hpp

#include "ChunkInfo.hpp"
#include "ReadingBytes.hpp"
#include "Exceptions.hpp"

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <cassert>

namespace wav
{

// 32kB default (requested) buffer size
inline constexpr size_t defaultRequestedReadBufferSize = 32768;

// Returns pair containing interleaved audio vector and Info.
// Calls in-place Read() overload.
// May throw wav::ReadError.
template <FloatOrDouble T>
std::pair <std::vector<T>, struct Info>
ReadInterleaved (std::string const & filepath,
                 size_t requestedBufferSize = defaultRequestedReadBufferSize);

// Returns pair containing audio separated into channels and Info.
// Calls in-place Read() overload.
// May throw wav::ReadError.
template <FloatOrDouble T>
std::pair <std::vector<std::vector<T>>, struct Info>
Read (std::string const & filepath,
      size_t requestedBufferSize = defaultRequestedReadBufferSize);


// In-place read of interleaved audio and Info.
// May throw wav::ReadError.
template <FloatOrDouble T>
void ReadInterleaved (std::string const & filepath,
                      std::vector<T> & destination,
                      struct Info & info,
                      size_t requestedBufferSize = defaultRequestedReadBufferSize);

// In-place read to separate channels and Info.
// May throw wav::ReadError.
template <FloatOrDouble T>
void Read (std::string const & filepath,
           std::vector<std::vector<T>> & destination,
           struct Info & info,
           size_t requestedBufferSize = defaultRequestedReadBufferSize);

struct Info
{
    // playback
    int channels {0};
    int sampleRate {0};
    int frames {0};
    // total samples = channels * frames
    // length in seconds = frames / sampleRate

    // sample encoding
    int bitDepth {0}; // bits per sample
    int containerSizeInBits {0}; // sample containerSize >= sample bitDepth
    bool isLinearPCM {false};
    bool isFloatPCM {false};

    void ExtractFrom (ChunkInfo const &);
};
std::ostream & operator<< (std::ostream & os, Info const &);

//////////////////////////////////////////////////////////////////

template <FloatOrDouble T>
std::pair <std::vector<T>, struct Info>
ReadInterleaved (std::string const & filepath,
                 size_t requestedBufferSize)
{
    std::vector<T> data;
    Info info;
    ReadInterleaved (filepath, data, info, requestedBufferSize);
    return {std::move (data), info};
}

// De-interleaved version
template <FloatOrDouble T>
std::pair <std::vector<std::vector<T>>, struct Info>
Read (std::string const & filepath,
      size_t requestedBufferSize)
{
    std::vector<std::vector<T>> data;
    Info info;
    Read (filepath, data, info, requestedBufferSize);
    return {std::move (data), info};
}

template <FloatOrDouble T>
void ReadInterleaved (std::string const & filepath,
                      std::vector<T> & destination,
                      Info & info,
                      size_t requestedBufferSize)
{
    ChunkInfo chunkInfo {filepath};

    const long fileLength = std::filesystem::file_size (filepath);
    if (chunkInfo.riff.size + 8 != fileLength)
    {
        throw ReadError ("RIFF chunk size + 8 does not equal file size for " + filepath);
    }

    info.ExtractFrom (chunkInfo);
    ReadData (filepath, destination, info, chunkInfo.data, requestedBufferSize);
}

template <FloatOrDouble T>
void Read (std::string const & filepath,
           std::vector<std::vector<T>> & destination,
           struct Info & info,
           size_t requestedBufferSize)
{
    ChunkInfo chunkInfo {filepath};

    const long fileLength = std::filesystem::file_size (filepath);
    if (chunkInfo.riff.size + 8 != fileLength)
    {
        throw ReadError ("RIFF chunk size + 8 does not equal file size for " + filepath);
    }

    info.ExtractFrom (chunkInfo);
    ReadData (filepath, destination, info, chunkInfo.data, requestedBufferSize);
}

template <FloatOrDouble T>
void ReadData (std::string const & filepath,
               std::vector<T> & destination,
               Info const & info,
               DataChunk const & data,
               size_t requestedBufferSizeInBytes)
{
    if (! std::filesystem::is_regular_file (filepath))
    {
        throw ReadError ("File does not exist: " + filepath);
    }

    std::ifstream in {filepath, std::ios::binary};
    if (!in) throw ReadError ("Failed to open: " + filepath);

    // Advance to the beginning of the data portion of the data chunk:
    in.seekg (data.offset);
    if (!in) throw ReadError ("Failed to advance to data portion of data chunk.");

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
        if (!in.read (reinterpret_cast<char*> (buffer.data()), bytesToRead))
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
void ReadData (std::string const & filepath,
               std::vector <std::vector<T>> & destination,
               Info const & info,
               DataChunk const & data,
               size_t requestedBufferSizeInBytes)
{
    if (! std::filesystem::is_regular_file (filepath))
    {
        throw ReadError ("File does not exist: " + filepath);
    }

    std::ifstream in {filepath, std::ios::binary};
    if (!in) throw ReadError ("Failed to open: " + filepath);

    // Advance to the beginning of the data portion of the data chunk:
    in.seekg (data.offset);
    if (!in) throw ReadError ("Failed to advance to data portion of data chunk.");

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
        if (!in.read (reinterpret_cast<char*> (buffer.data()), bytesToRead))
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

template <typename T> T powerOfTwo (int power);

template <FloatOrDouble T>
void ReadLinearPCM (std::vector<std::byte> const & buffer,
                    std::vector<T> & destination,
                    Info const & info,
                    size_t samplesToRead,
                    size_t writePosition)
{
    // Normalize signed integers to [-1.0, 1.0) by dividing by 2^(bits-1).
    // Example:
    // 16-bit range: [-32768, 32767], divided by 32768: [-1.0, 0.9999694824]
    const auto two_to_the_n_minus_one  {powerOfTwo<T> (info.bitDepth - 1)};

    size_t pos {0};
    switch (info.containerSizeInBits)
    {
        case 8:
            for (size_t n = writePosition; n < samplesToRead + writePosition; ++n)
            {
                destination [n] = (uint8 (buffer, pos) - 128.) / two_to_the_n_minus_one;
            }
            break;

        case 16:
            for (size_t n = writePosition; n < samplesToRead + writePosition; ++n)
            {
                destination [n] = int16_le (buffer, pos) / two_to_the_n_minus_one;
            }
            break;

        case 24:
            for (size_t n = writePosition; n < samplesToRead + writePosition; ++n)
                destination [n] = int24_le (buffer, pos) / two_to_the_n_minus_one;
            break;

        case 32:
            for (size_t n = writePosition; n < samplesToRead + writePosition; ++n)
                destination [n] = int32_le (buffer, pos) / two_to_the_n_minus_one;
            break;
    }
}

template <FloatOrDouble T>
void ReadLinearPCM (std::vector<std::byte> const & buffer,
                    std::vector<std::vector<T>> & destination,
                    Info const & info,
                    size_t framesToRead,
                    size_t writePosition)
{
    // Normalize signed integers to [-1.0, 1.0) by dividing by 2^(bits-1).
    // Example:
    // 16-bit range: [-32768, 32767], divided by 32768: [-1.0, 0.9999694824]
    const auto two_to_the_n_minus_one  {powerOfTwo<T> (info.bitDepth - 1)};

    size_t pos {0};
    switch (info.containerSizeInBits)
    {
        case 8:
            for (size_t frame = writePosition; frame < writePosition + framesToRead; ++frame)
            {
                for (size_t channel = 0; channel < info.channels; ++channel)
                {
                    destination [channel][frame] = (uint8 (buffer, pos) - 128.) / two_to_the_n_minus_one;
                }
            }
            break;

        case 16:
            for (size_t frame = writePosition; frame < writePosition + framesToRead; ++frame)
            {
                for (size_t channel = 0; channel < info.channels; ++channel)
                {
                    destination [channel][frame] = int16_le (buffer, pos) / two_to_the_n_minus_one;
                }
            }
            break;

        case 24:
            for (size_t frame = writePosition; frame < writePosition + framesToRead; ++frame)
            {
                for (size_t channel = 0; channel < info.channels; ++channel)
                {
                    destination [channel][frame] = int24_le (buffer, pos) / two_to_the_n_minus_one;
                }
            }
            break;

        case 32:
            for (size_t frame = writePosition; frame < writePosition + framesToRead; ++frame)
            {
                for (size_t channel = 0; channel < info.channels; ++channel)
                {
                    destination [channel][frame] = int32_le (buffer, pos) / two_to_the_n_minus_one;
                }
            }
            break;
    }
}

template <FloatOrDouble T>
void ReadFloatPCM (std::vector<std::byte> const & buffer,
                   std::vector<T> & destination,
                   Info const & info,
                   size_t samplesToRead,
                   size_t writePosition)
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

template <typename T>
T powerOfTwo (int power)
{
    // Must be positive. Must not shift past the MSB.
    assert (power > 0 && power < 64);
    return static_cast<T> (static_cast<uint64_t>(1) << power);
}

} // namespace wav

#endif
