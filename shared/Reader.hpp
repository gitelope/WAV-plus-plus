
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
#include <cassert>
#include <vector>
#include <cstdio>
#include <bit>
#include <string>
#include <algorithm>
#include <memory>

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
    ,readBufferSize (bufferSizeKB * 1024)
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

    // Read interleaved samples into user-provided buffer.
    template <FloatOrDouble T>
    size_t readInterleaved (T * destination, size_t samplesToRead);

    // Get file information.
    Info const & getInfo() const { return info; }

private:

    std::unique_ptr<FILE, decltype(&fclose)> file;
    Info info;
    DataChunk dataChunk;
    size_t readBufferSize;        // in bytes
    uint32_t dataPosition {0};    // current byte position in data chunk
    uint32_t dataChunkLength {0}; // in bytes

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
size_t Reader::readInterleaved (T * destination, size_t samplesRequested)
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
            destination [writePosition + i] = (buffer [i] - uint8Offset) / uint8MaxAbs;
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
            destination [writePosition + i] = buffer [i] / int16MaxAbs;
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
            destination [writePosition + i] = buffer [i] / int24MaxAbs;
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
            destination [writePosition + i] = buffer [i] / int32MaxAbs;
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
            case 8:  bufferLPCMU8.resize (readBufferSize);     break;
            case 16: bufferLPCM16.resize (readBufferSize / 2); break;
            case 24: bufferLPCM24.resize (readBufferSize / 3); break;
            case 32: bufferLPCM32.resize (readBufferSize / 4); break;
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
            bufferFloatPCM32.resize (readBufferSize / 4);
        }
        else if (info.bitDepth == 64)
        {
            // Only used for double->float conversion:
            bufferFloatPCM64.resize (readBufferSize / 8);
        }
    }
}

} // namespace wav

#endif
