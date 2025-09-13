
#ifndef wavplusplus_Read_hpp
#define wavplusplus_Read_hpp

#include "ChunkInfo.hpp"
#include "Info.hpp"
#include "Exceptions.hpp"
#include "DequantizationConstants.hpp"
#include "ReadDataInterleaved.hpp"
#include "ReadDataSeparateChannels.hpp"

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
