
#ifndef wavplusplus_Reader_hpp
#define wavplusplus_Reader_hpp

#include "Formats.hpp"
#include "Info.hpp"
#include "ChunkInfo.hpp"
#include "ReadingBytes.hpp"
#include "DequantizationConstants.hpp"
#include "BufferSizing.hpp"
#include "int24.hpp"
#include "Exceptions.hpp"

#include <algorithm>
#include <bit>
#include <cassert>
#include <climits>
#include <cstdio>
#include <cstdint> 
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

static_assert (sizeof (float) == 4);
static_assert (sizeof (double) == 8);
static_assert (CHAR_BIT == 8);

namespace wav
{

class Reader
{

public:

    explicit Reader (std::string const & filepath, size_t bufferSizeKB = 32)
    :file (fopen (filepath.c_str(), "rb"), fclose)
    ,readBufferSizeInBytes (bufferSizeKB * 1024)
    {
        // Check if file opened successfully
        if (!file)
        {
            throw ReadError ("Failed to open file: " + filepath);
        }

        // Parse file header
        ChunkInfo chunkInfo {file.get()};
        info.ExtractFrom (chunkInfo);
        dataChunk = chunkInfo.data;

        // Set up data chunk information
        dataChunkLength = dataChunk.size;

        // Position file at start of data
        if (fsetpos (file.get(), &dataChunk.offset) != 0)
        {
            throw ReadError ("Failed to advance to data portion of data chunk.");
        }

        InitializeBuffer();
    }

    // Fill user-provided buffer with interleaved samples.
    // Returns the number of samples read.
    // May throw wav::ReadError.
    template <FloatOrDouble T>
    size_t read (T * destination, size_t samplesRequested);

    // read() convenience overloads
    //
    // fixed-size C-style array (fill with samples)
    // May throw wav::ReadError.
    template <FloatOrDouble T, size_t N>
    size_t read (T (&destination)[N]);
    //
    // Main overload of read() is called for all C-style arrays,
    // so both fixed-sized and dynamically-sized,
    // when samples requested is provided.
    //
    // std::vector (fill with samples)
    // May throw wav::ReadError.
    template <FloatOrDouble T>
    size_t read (std::vector<T> & destination);
    //
    // std::vector (fill with number of samples requested)
    // May throw wav::ReadError.
    template <FloatOrDouble T>
    size_t read (std::vector<T> & destination, size_t samplesRequested);
    //
    // std::array (fill with samples)
    // May throw wav::ReadError.
    template <FloatOrDouble T, size_t N>
    size_t read (std::array<T,N> & destination);
    //
    // std::array (fill with number of samples requested)
    // May throw wav::ReadError.
    template <FloatOrDouble T, size_t N>
    size_t read (std::array<T,N> & destination, size_t samplesRequested);

    // A convenience function that calls read.
    // Returns the number of whole frames read.
    // May throw wav::ReadError.
    template <FloatOrDouble T>
    size_t readFrames (T * destination, size_t framesRequested);

    // Get file information.
    Info const & getInfo() const {return info;}
    uint16_t channels() const {return info.channels;}
    uint32_t frames() const {return info.frames;}
    uint32_t samples() const {return info.channels * info.frames;}
    uint32_t sampleRate() const {return info.sampleRate;}
    uint16_t bitDepth() const {return info.bitDepth;}
    uint16_t containerSizeInBits() const {return info.containerSizeInBits;}
    bool isLinearPCM() const {return info.isLinearPCM;}
    bool isFloatPCM() const {return info.isFloatPCM;}

private:

    std::unique_ptr<FILE, decltype(&fclose)> file;
    Info info;

    DataChunk dataChunk;
    uint32_t dataPosition {0};    // current byte position in data chunk
    uint32_t dataChunkLength {0}; // in bytes

    size_t readBufferSizeInBytes;

    // Internal buffers for different encodings
    std::vector<uint8_t> bufferLPCMU8;
    std::vector<int16_t> bufferLPCM16;
    std::vector<int24>   bufferLPCM24;
    std::vector<int32_t> bufferLPCM32;
    std::vector<float>   bufferFloatPCM32;
    std::vector<double>  bufferFloatPCM64;

    void InitializeBuffer();

    // floating-point PCM
    template <FloatOrDouble T> void ReadFloatPCMNoConversion (T * destination, size_t samplesToRead);
    void ReadFloatToDouble (double * destination, size_t samplesToRead);
    void ReadDoubleToFloat (float * destination, size_t samplesToRead);

    // linear PCM
    template <FloatOrDouble T> void ReadLPCMU8 (T * destination, size_t samplesToRead);
    template <FloatOrDouble T> void ReadLPCM16 (T * destination, size_t samplesToRead);
    template <FloatOrDouble T> void ReadLPCM24 (T * destination, size_t samplesToRead);
    template <FloatOrDouble T> void ReadLPCM32 (T * destination, size_t samplesToRead);
};

template <FloatOrDouble T>
size_t Reader::read (T * destination, size_t samplesRequested)
{
    if (!file || samplesRequested == 0) return 0;

    // Calculate how many samples to read.
    const size_t samplesRemaining {(dataChunkLength - dataPosition) / (info.bitDepth / 8)};
    const size_t samplesToRead {std::min (samplesRequested, samplesRemaining)};
    if (samplesToRead == 0) return 0;

    if (info.isLinearPCM)
    {
        switch (info.containerSizeInBits)
        {
            case 8:  ReadLPCMU8 (destination, samplesToRead); break;
            case 16: ReadLPCM16 (destination, samplesToRead); break;
            case 24: ReadLPCM24 (destination, samplesToRead); break;
            case 32: ReadLPCM32 (destination, samplesToRead); break;
            default: throw ReadError ("Unsupported LPCM container size: " + std::to_string (info.containerSizeInBits));
        }
    }
    else if (info.isFloatPCM)
    {
        if constexpr (std::is_same_v<T, float>)
        {
            switch (info.bitDepth)
            {
                case 32: ReadFloatPCMNoConversion (destination, samplesToRead); break;
                case 64: ReadDoubleToFloat (destination, samplesToRead);        break;
                default: throw ReadError ("Unsupported float PCM bit depth: " + std::to_string (info.bitDepth));
            }
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            switch (info.bitDepth)
            {
                case 32: ReadFloatToDouble (destination, samplesToRead);        break;
                case 64: ReadFloatPCMNoConversion (destination, samplesToRead); break;
                default: throw ReadError ("Unsupported float PCM bit depth: " + std::to_string (info.bitDepth));
            }
        }
    }

    dataPosition += samplesToRead * (info.bitDepth / 8);

    return samplesToRead;
}

// fixed-size array
template <FloatOrDouble T, size_t N>
size_t Reader::read (T (&destination)[N])
{
    return read (destination, N);
}

// std::vector
template <FloatOrDouble T>
size_t Reader::read (std::vector<T> & destination)
{
    return read (destination.data(), destination.size());
}

// std::vector
template <FloatOrDouble T>
size_t Reader::read (std::vector<T> & destination, size_t samplesRequested)
{
    return read (destination.data(), std::min (samplesRequested, destination.size()));
}

// std::array
template <FloatOrDouble T, size_t N>
size_t Reader::read (std::array<T,N> & destination)
{
    return read (destination.data(), N);
}

// std::array
template <FloatOrDouble T, size_t N>
size_t Reader::read (std::array<T,N> & destination, size_t samplesRequested)
{
    return read (destination.data(), std::min (samplesRequested, N));
}

template <FloatOrDouble T>
size_t Reader::readFrames (T * destination, size_t framesRequested)
{
    return read (destination, framesRequested * info.channels) / info.channels;
}

template <FloatOrDouble T>
void Reader::ReadFloatPCMNoConversion (T * destination, size_t samplesToRead)
{
    // Input bit depth same as output: float->float or double->double.
    // Read directly from file to destination.
    if (fread (destination, sizeof (T), samplesToRead, file.get()) != samplesToRead)
    {
        throw ReadError ("Failed to read buffer from file.");
    }

    // Byteswap if needed
    if constexpr (std::endian::native == std::endian::big)
    {
        for (size_t i = 0; i < samplesToRead; ++i)
        {
            if constexpr (std::is_same_v<T, float>)
            {
                destination [i] = std::bit_cast<float> (std::byteswap (std::bit_cast<uint32_t> (destination [i])));
            }
            else // double
            {
                destination [i] = std::bit_cast<double> (std::byteswap (std::bit_cast<uint64_t> (destination [i])));
            }
        }
    }
}

void Reader::ReadFloatToDouble (double * destination, size_t totalSamplesToRead)
{
    // float -> double
    auto & buffer {bufferFloatPCM32};
    constexpr size_t bytesPerSample {4};

    size_t samplesRemaining {totalSamplesToRead};
    size_t writePosition {0};

    while (samplesRemaining)
    {
        const size_t samplesToRead {std::min (samplesRemaining, buffer.size())};

        if (fread (buffer.data(), bytesPerSample, samplesToRead, file.get()) != samplesToRead)
        {
            throw ReadError ("Failed to read buffer from file.");
        }

        // Byteswap if needed
        if constexpr (std::endian::native == std::endian::big)
        {
            for (size_t i = 0; i < samplesToRead; ++i)
            {
                buffer [i] = std::bit_cast<float> (std::byteswap (std::bit_cast<uint32_t> (buffer [i])));
            }
        }

        // Convert to destination type
        for (size_t i = 0; i < samplesToRead; ++i)
        {
            destination [writePosition + i] = static_cast<double> (buffer [i]);
        }

        writePosition += samplesToRead;
        samplesRemaining -= samplesToRead;
    }
}

void Reader::ReadDoubleToFloat (float * destination, size_t totalSamplesToRead)
{
    // double -> float
    auto & buffer {bufferFloatPCM64};
    constexpr size_t bytesPerSample {8};

    size_t samplesRemaining {totalSamplesToRead};
    size_t writePosition {0};

    while (samplesRemaining)
    {
        const size_t samplesToRead {std::min (samplesRemaining, buffer.size())};

        if (fread (buffer.data(), bytesPerSample, samplesToRead, file.get()) != samplesToRead)
        {
            throw ReadError ("Failed to read buffer from file.");
        }

        // Byteswap if needed
        if constexpr (std::endian::native == std::endian::big)
        {
            for (size_t i = 0; i < samplesToRead; ++i)
            {
                buffer [i] = std::bit_cast<double> (std::byteswap (std::bit_cast<uint64_t> (buffer [i])));
            }
        }

        // Convert to destination type
        for (size_t i = 0; i < samplesToRead; ++i)
        {
            destination [writePosition + i] = static_cast<float> (buffer [i]);
        }

        writePosition += samplesToRead;
        samplesRemaining -= samplesToRead;
    }
}

template <FloatOrDouble T>
void Reader::ReadLPCMU8 (T * destination, size_t totalSamplesToRead)
{
    auto & buffer {bufferLPCMU8};
    constexpr size_t bytesPerSample {1};

    constexpr auto offset {static_cast<T> (uint8DequantizationOffset)};
    constexpr auto max    {static_cast<T> (uint8MaxAbs)};

    size_t samplesRemaining {totalSamplesToRead};
    size_t writePosition {0};

    while (samplesRemaining)
    {
        const size_t samplesToRead {std::min (samplesRemaining, buffer.size())};

        if (fread (buffer.data(), bytesPerSample, samplesToRead, file.get()) != samplesToRead)
        {
            throw ReadError ("Failed to read buffer from file.");
        }

        // Convert to destination type
        for (size_t i = 0; i < samplesToRead; ++i)
        {
            destination [writePosition + i] = (buffer [i] - offset) / max;
        }

        writePosition += samplesToRead;
        samplesRemaining -= samplesToRead;
    }
}

template <FloatOrDouble T>
void Reader::ReadLPCM16 (T * destination, size_t totalSamplesToRead)
{
    auto & buffer {bufferLPCM16};
    constexpr size_t bytesPerSample {2};

    constexpr auto max {static_cast<T> (int16MaxAbs)};

    size_t samplesRemaining {totalSamplesToRead};
    size_t writePosition {0};

    while (samplesRemaining)
    {
        const size_t samplesToRead {std::min (samplesRemaining, buffer.size())};

        if (fread (buffer.data(), bytesPerSample, samplesToRead, file.get()) != samplesToRead)
        {
            throw ReadError ("Failed to read buffer from file.");
        }

        // Byteswap if needed
        if constexpr (std::endian::native == std::endian::big)
        {
            for (size_t i = 0; i < samplesToRead; ++i)
            {
                buffer [i] = std::byteswap (buffer [i]);
            }
        }

        // Convert to destination type
        for (size_t i = 0; i < samplesToRead; ++i)
        {
            destination [writePosition + i] = buffer [i] / max;
        }

        writePosition += samplesToRead;
        samplesRemaining -= samplesToRead;
    }
}

template <FloatOrDouble T>
void Reader::ReadLPCM24 (T * destination, size_t totalSamplesToRead)
{
    auto & buffer {bufferLPCM24};
    constexpr size_t bytesPerSample {3};

    constexpr auto max {static_cast<T> (int24MaxAbs)};

    size_t samplesRemaining {totalSamplesToRead};
    size_t writePosition {0};

    while (samplesRemaining)
    {
        const size_t samplesToRead {std::min (samplesRemaining, buffer.size())};

        if (fread (buffer.data(), bytesPerSample, samplesToRead, file.get()) != samplesToRead)
        {
            throw ReadError ("Failed to read buffer from file.");
        }

        // Byteswap if needed
        if constexpr (std::endian::native == std::endian::big)
        {
            for (size_t i = 0; i < samplesToRead; ++i)
            {
                std::swap (buffer[i].bytes[0], buffer[i].bytes[2]);
            }
        }

        // Convert to destination type
        for (size_t i = 0; i < samplesToRead; ++i)
        {
            destination [writePosition + i] = buffer [i] / max;
        }

        writePosition += samplesToRead;
        samplesRemaining -= samplesToRead;
    }
}

template <FloatOrDouble T>
void Reader::ReadLPCM32 (T * destination, size_t totalSamplesToRead)
{
    auto & buffer {bufferLPCM32};
    constexpr size_t bytesPerSample {4};

    constexpr auto max {static_cast<T> (int32MaxAbs)};

    size_t samplesRemaining {totalSamplesToRead};
    size_t writePosition {0};

    while (samplesRemaining)
    {
        const size_t samplesToRead {std::min (samplesRemaining, buffer.size())};

        if (fread (buffer.data(), bytesPerSample, samplesToRead, file.get()) != samplesToRead)
        {
            throw ReadError ("Failed to read buffer from file.");
        }

        // Byteswap if needed
        if constexpr (std::endian::native == std::endian::big)
        {
            for (size_t i = 0; i < samplesToRead; ++i)
            {
                buffer [i] = std::byteswap (buffer [i]);
            }
        }

        // Convert to destination type
        for (size_t i = 0; i < samplesToRead; ++i)
        {
            destination [writePosition + i] = buffer [i] / max;
        }

        writePosition += samplesToRead;
        samplesRemaining -= samplesToRead;
    }
}

inline void Reader::InitializeBuffer()
{
    // Only initialize the buffer(s) needed for the current file format
    if (info.isLinearPCM)
    {
        switch (info.containerSizeInBits)
        {
            case 8:  bufferLPCMU8.resize (readBufferSizeInBytes);     break;
            case 16: bufferLPCM16.resize (readBufferSizeInBytes / 2); break;
            case 24: bufferLPCM24.resize (readBufferSizeInBytes / 3); break;
            case 32: bufferLPCM32.resize (readBufferSizeInBytes / 4); break;
        }
    }
    else if (info.isFloatPCM)
    {
        // We won't need a buffer if the encoding format matches the
        // decoding format, i.e. double -> double or float -> float,
        // but we don't yet know what the decoding format will be, so we
        // initialize a buffer just in case.
        if (info.bitDepth == 32)
        {
            // Only used for float->double conversion:
            bufferFloatPCM32.resize (readBufferSizeInBytes / 4);
        }
        else if (info.bitDepth == 64)
        {
            // Only used for double->float conversion:
            bufferFloatPCM64.resize (readBufferSizeInBytes / 8);
        }
    }
}

} // namespace wav

#endif
