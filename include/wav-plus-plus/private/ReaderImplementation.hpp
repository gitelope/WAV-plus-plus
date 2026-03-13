
#ifndef wavplusplus_ReaderImplementation_hpp
#define wavplusplus_ReaderImplementation_hpp

#include "BufferSizing.hpp"

#include <algorithm>
#include <bit>
#include <cassert>
#include <type_traits>

namespace wav
{

Reader::Reader (std::string const & filepath, uint64_t suggestedBufferSizeKB)
:file (fopen (filepath.c_str(), "rb"), fclose)
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

    // Position file at start of data
    if (fsetpos (file.get(), &dataChunk.offset) != 0)
    {
        throw ReadError ("Failed to advance to data portion of data chunk.");
    }

    InitializeBuffer (suggestedBufferSizeKB);
}

template <FloatOrDouble T>
size_t Reader::read (T * destination, size_t samplesRequested)
{
    if (!file || samplesRequested == 0) return 0;

    // Calculate how many samples to read.
    const size_t bytesPerSample {containerSizeInBits() / 8U};
    const size_t samplesRemaining {(dataChunk.size - dataPosition) / bytesPerSample};
    const size_t samplesToRead {std::min (samplesRequested, samplesRemaining)};
    if (samplesToRead == 0) return 0;

    if (isLinearPCM())
    {
        switch (containerSizeInBits())
        {
            case 8:  ReadLPCMU8 (destination, samplesToRead); break;
            case 16: ReadLPCM16 (destination, samplesToRead); break;
            case 24: ReadLPCM24 (destination, samplesToRead); break;
            case 32: ReadLPCM32 (destination, samplesToRead); break;
            default: throw ReadError ("Unsupported LPCM container size: " + std::to_string (containerSizeInBits()));
        }
    }
    else if (isFloatPCM())
    {
        assert (bitDepth() == containerSizeInBits());

        if constexpr (std::is_same_v<T, float>)
        {
            switch (bitDepth())
            {
                case 32: ReadFloatPCMNoConversion (destination, samplesToRead); break;
                case 64: ReadDoubleToFloat (destination, samplesToRead);        break;
                default: throw ReadError ("Unsupported float PCM bit depth: " + std::to_string (bitDepth()));
            }
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            switch (bitDepth())
            {
                case 32: ReadFloatToDouble (destination, samplesToRead);        break;
                case 64: ReadFloatPCMNoConversion (destination, samplesToRead); break;
                default: throw ReadError ("Unsupported float PCM bit depth: " + std::to_string (bitDepth()));
            }
        }
    }

    dataPosition += samplesToRead * bytesPerSample;

    return samplesToRead;
}

template <FloatOrDouble T>
size_t Reader::read (std::vector<T> & destination)
{
    return read (destination.data(), destination.size());
}

template <FloatOrDouble T>
size_t Reader::read (std::vector<T> & destination, size_t samplesRequested)
{
    if (samplesRequested > destination.size())
    {
        throw ReadError ("samples requested > destination size");
    }
    return read (destination.data(), samplesRequested);
}

template <FloatOrDouble T>
size_t Reader::readFrames (T * destination, size_t framesRequested)
{
    return read (destination, framesRequested * channels()) / channels();
}

template <FloatOrDouble T>
size_t Reader::readFrames (std::vector<T> & destination)
{
    if (destination.size() % channels() != 0)
    {
        throw ReadError ("destination size in samples is not a whole number of frames");
    }
    return read (destination, destination.size()) / channels();
}

template <FloatOrDouble T>
size_t Reader::readFrames (std::vector<T> & destination, size_t framesRequested)
{
    if (framesRequested * channels() > destination.size())
    {
        throw ReadError ("frames requested > destination size");
    }
    return read (destination, framesRequested * channels()) / channels();
}

template <FloatOrDouble T>
void Reader::ReadLPCMU8 (T * destination, size_t totalSamplesToRead)
{
    auto & buffer {bufferLPCMU8};
    constexpr size_t bytesPerSample {1};

    constexpr double offset {uint8DequantizationOffset};
    const double scale      {normalization (bitDepth())};

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
            destination [writePosition + i] = static_cast<T> ((buffer [i] - offset) / scale);
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

    const double scale {normalization (bitDepth())};

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
            destination [writePosition + i] = static_cast<T> (buffer [i] / scale);
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

    const double scale {normalization (bitDepth())};

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
            destination [writePosition + i] = static_cast<T> (buffer [i] / scale);
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

    const double scale {normalization (bitDepth())};

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
            destination [writePosition + i] = static_cast<T> (buffer [i] / scale);
        }

        writePosition += samplesToRead;
        samplesRemaining -= samplesToRead;
    }
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

inline void Reader::ReadFloatToDouble (double * destination, size_t totalSamplesToRead)
{
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

inline void Reader::ReadDoubleToFloat (float * destination, size_t totalSamplesToRead)
{
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
size_t Reader::read (T ** const destinations, size_t numChannels, size_t framesRequested)
{
    if (!file || framesRequested == 0 || numChannels == 0) return 0;
    if (numChannels > channels())
    {
        throw ReadError ("Requested more channels than available.");
    }

    const size_t bytesPerSample {containerSizeInBits() / 8U};
    const size_t samplesRemaining {(dataChunk.size - dataPosition) / bytesPerSample};
    const size_t framesRemaining {samplesRemaining / channels()};
    const size_t framesToRead {std::min (framesRequested, framesRemaining)};
    if (framesToRead == 0) return 0;

    if (isLinearPCM())
    {
        switch (containerSizeInBits())
        {
            case 8:  ReadLPCMU8 (destinations, numChannels, framesToRead); break;
            case 16: ReadLPCM16 (destinations, numChannels, framesToRead); break;
            case 24: ReadLPCM24 (destinations, numChannels, framesToRead); break;
            case 32: ReadLPCM32 (destinations, numChannels, framesToRead); break;
            default: throw ReadError ("Unsupported LPCM container size: " + std::to_string (containerSizeInBits()));
        }
    }
    else if (isFloatPCM())
    {
        assert (bitDepth() == containerSizeInBits());

        switch (bitDepth())
        {
            case 32: ReadFloatPCM32 (destinations, numChannels, framesToRead); break;
            case 64: ReadFloatPCM64 (destinations, numChannels, framesToRead); break;
            default: throw ReadError ("Unsupported float PCM bit depth: " + std::to_string (bitDepth()));
        }
    }

    dataPosition += framesToRead * channels() * bytesPerSample;
    return framesToRead;
}

template <FloatOrDouble T>
void Reader::ReadLPCMU8 (T ** const destinations, size_t channelsToRead, size_t totalFramesToRead)
{
    auto & buffer {bufferLPCMU8};
    constexpr size_t bytesPerSample {1};

    const auto sourceChannels {channels()};
    assert (channelsToRead <= sourceChannels);

    const size_t framesPerBuffer {buffer.size() / sourceChannels};
    assert (framesPerBuffer > 0);

    const double offset {uint8DequantizationOffset};
    const double scale  {normalization (bitDepth())};

    size_t framesRemaining {totalFramesToRead};
    size_t baseFrame {0};

    while (framesRemaining)
    {
        const size_t framesThisChunk {std::min (framesRemaining, framesPerBuffer)};
        const size_t samplesThisChunk {framesThisChunk * sourceChannels};

        if (fread (buffer.data(), bytesPerSample, samplesThisChunk, file.get()) != samplesThisChunk)
        {
            throw ReadError ("Failed to read buffer from file.");
        }

        for (size_t frame = 0; frame < framesThisChunk; ++frame)
        {
            const size_t baseIndex {frame * sourceChannels};
            for (size_t channel = 0; channel < channelsToRead; ++channel)
            {
                const double normalized {(buffer [baseIndex + channel] - offset) / scale};
                destinations [channel] [baseFrame + frame] = static_cast<T> (normalized);
            }
        }

        baseFrame       += framesThisChunk;
        framesRemaining -= framesThisChunk;
    }
}

template <FloatOrDouble T>
void Reader::ReadLPCM16 (T ** const destinations, size_t channelsToRead, size_t totalFramesToRead)
{
    auto & buffer {bufferLPCM16};
    constexpr size_t bytesPerSample {2};

    const auto sourceChannels {channels()};
    assert (channelsToRead <= sourceChannels);

    const size_t framesPerBuffer {buffer.size() / sourceChannels};
    assert (framesPerBuffer > 0);

    const double scale {normalization (bitDepth())};

    size_t framesRemaining {totalFramesToRead};
    size_t baseFrame {0};

    while (framesRemaining)
    {
        const size_t framesThisChunk {std::min (framesRemaining, framesPerBuffer)};
        const size_t samplesThisChunk {framesThisChunk * sourceChannels};

        if (fread (buffer.data(), bytesPerSample, samplesThisChunk, file.get()) != samplesThisChunk)
        {
            throw ReadError ("Failed to read buffer from file.");
        }

        if constexpr (std::endian::native == std::endian::big)
        {
            for (size_t i = 0; i < samplesThisChunk; ++i)
            {
                buffer[i] = std::byteswap (buffer[i]);
            }
        }

        for (size_t frame = 0; frame < framesThisChunk; ++frame)
        {
            const size_t baseIndex {frame * sourceChannels};
            for (size_t channel = 0; channel < channelsToRead; ++channel)
            {
                const double normalized {buffer [baseIndex + channel] / scale};
                destinations [channel] [baseFrame + frame] = static_cast<T> (normalized);
            }
        }

        baseFrame       += framesThisChunk;
        framesRemaining -= framesThisChunk;
    }
}

template <FloatOrDouble T>
void Reader::ReadLPCM24 (T ** const destinations, size_t channelsToRead, size_t totalFramesToRead)
{
    auto & buffer {bufferLPCM24};
    constexpr size_t bytesPerSample {3};

    const auto sourceChannels {channels()};
    assert (channelsToRead <= sourceChannels);

    const size_t framesPerBuffer {buffer.size() / sourceChannels};
    assert (framesPerBuffer > 0);

    const double scale {normalization (bitDepth())};

    size_t framesRemaining {totalFramesToRead};
    size_t baseFrame {0};

    while (framesRemaining)
    {
        const size_t framesThisChunk {std::min (framesRemaining, framesPerBuffer)};
        const size_t samplesThisChunk {framesThisChunk * sourceChannels};

        if (fread (buffer.data(), bytesPerSample, samplesThisChunk, file.get()) != samplesThisChunk)
        {
            throw ReadError ("Failed to read buffer from file.");
        }

        if constexpr (std::endian::native == std::endian::big)
        {
            for (size_t i = 0; i < samplesThisChunk; ++i)
            {
                std::swap (buffer[i].bytes[0], buffer[i].bytes[2]);
            }
        }

        for (size_t frame = 0; frame < framesThisChunk; ++frame)
        {
            const size_t baseIndex {frame * sourceChannels};
            for (size_t channel = 0; channel < channelsToRead; ++channel)
            {
                const double normalized {static_cast<int32_t> (buffer [baseIndex + channel]) / scale};
                destinations [channel] [baseFrame + frame] = static_cast<T> (normalized);
            }
        }

        baseFrame       += framesThisChunk;
        framesRemaining -= framesThisChunk;
    }
}

template <FloatOrDouble T>
void Reader::ReadLPCM32 (T ** const destinations, size_t channelsToRead, size_t totalFramesToRead)
{
    auto & buffer {bufferLPCM32};
    constexpr size_t bytesPerSample {4};

    const auto sourceChannels {channels()};
    assert (channelsToRead <= sourceChannels);

    const size_t framesPerBuffer {buffer.size() / sourceChannels};
    assert (framesPerBuffer > 0);

    const double scale {normalization (bitDepth())};

    size_t framesRemaining {totalFramesToRead};
    size_t baseFrame {0};

    while (framesRemaining)
    {
        const size_t framesThisChunk {std::min (framesRemaining, framesPerBuffer)};
        const size_t samplesThisChunk {framesThisChunk * sourceChannels};

        if (fread (buffer.data(), bytesPerSample, samplesThisChunk, file.get()) != samplesThisChunk)
        {
            throw ReadError ("Failed to read buffer from file.");
        }

        if constexpr (std::endian::native == std::endian::big)
        {
            for (size_t i = 0; i < samplesThisChunk; ++i)
            {
                buffer[i] = std::byteswap (buffer[i]);
            }
        }

        for (size_t frame = 0; frame < framesThisChunk; ++frame)
        {
            const size_t baseIndex {frame * sourceChannels};
            for (size_t channel = 0; channel < channelsToRead; ++channel)
            {
                const double normalized {buffer [baseIndex + channel] / scale};
                destinations [channel] [baseFrame + frame] = static_cast<T> (normalized);
            }
        }

        baseFrame       += framesThisChunk;
        framesRemaining -= framesThisChunk;
    }
}

template <FloatOrDouble T>
void Reader::ReadFloatPCM32 (T ** const destinations, size_t channelsToRead, size_t totalFramesToRead)
{
    auto & buffer {bufferFloatPCM32};
    constexpr size_t bytesPerSample {4};

    const auto sourceChannels {channels()};
    assert (channelsToRead <= sourceChannels);

    const size_t framesPerBuffer {buffer.size() / sourceChannels};
    assert (framesPerBuffer > 0);

    size_t framesRemaining {totalFramesToRead};
    size_t baseFrame {0};

    while (framesRemaining)
    {
        const size_t framesThisChunk {std::min (framesRemaining, framesPerBuffer)};
        const size_t samplesThisChunk {framesThisChunk * sourceChannels};

        if (fread (buffer.data(), bytesPerSample, samplesThisChunk, file.get()) != samplesThisChunk)
        {
            throw ReadError ("Failed to read buffer from file.");
        }

        // Byteswap if needed. (Consider doing after deinterleaving instead of before.)
        if constexpr (std::endian::native == std::endian::big)
        {
            for (size_t i = 0; i < samplesThisChunk; ++i)
            {
                buffer[i] = std::bit_cast<float> (std::byteswap (std::bit_cast<uint32_t> (buffer[i])));
            }
        }

        // Iterate over buffer's frames.
        for (size_t frame = 0; frame < framesThisChunk; ++frame)
        {
            const size_t baseIndex {frame * sourceChannels};
            // Iterate over output channels.
            // This may be less than the total number of channels in the file.
            for (size_t channel = 0; channel < channelsToRead; ++channel)
            {
                destinations [channel] [baseFrame + frame] = static_cast<T> (buffer [baseIndex + channel]);
            }
        }

        baseFrame       += framesThisChunk;
        framesRemaining -= framesThisChunk;
    }
}

template <FloatOrDouble T>
void Reader::ReadFloatPCM64 (T ** const destinations, size_t channelsToRead, size_t totalFramesToRead)
{
    auto & buffer {bufferFloatPCM64};
    constexpr size_t bytesPerSample {8};

    const auto sourceChannels {channels()};
    assert (channelsToRead <= sourceChannels);

    const size_t framesPerBuffer {buffer.size() / sourceChannels};
    assert (framesPerBuffer > 0);

    size_t framesRemaining {totalFramesToRead};
    size_t baseFrame {0};

    while (framesRemaining)
    {
        const size_t framesThisChunk {std::min (framesRemaining, framesPerBuffer)};
        const size_t samplesThisChunk {framesThisChunk * sourceChannels};

        if (fread (buffer.data(), bytesPerSample, samplesThisChunk, file.get()) != samplesThisChunk)
        {
            throw ReadError ("Failed to read buffer from file.");
        }

        // Byteswap if needed. (Consider doing after deinterleaving instead of before.)
        if constexpr (std::endian::native == std::endian::big)
        {
            for (size_t i = 0; i < samplesThisChunk; ++i)
            {
                buffer[i] = std::bit_cast<double> (std::byteswap (std::bit_cast<uint64_t> (buffer[i])));
            }
        }

        // Iterate over buffer's frames.
        for (size_t frame = 0; frame < framesThisChunk; ++frame)
        {
            const size_t baseIndex {frame * sourceChannels};
            // Iterate over output channels.
            // This may be less than the total number of channels in the file.
            for (size_t channel = 0; channel < channelsToRead; ++channel)
            {
                destinations [channel] [baseFrame + frame] = static_cast<T> (buffer [baseIndex + channel]);
            }
        }

        baseFrame       += framesThisChunk;
        framesRemaining -= framesThisChunk;
    }
}

/// Initialize the buffer needed for the current file format.
inline void Reader::InitializeBuffer (uint64_t suggestedBufferSizeKB)
{
    // Size buffer so that it holds a whole number of frames, at least one frame,
    // and is no bigger than what would be required to read in the entire data chunk:
    const size_t size
    {
        bufferSize (suggestedBufferSizeKB, dataChunk.size, containerSizeInBits(), channels())
    };

    if (isLinearPCM())
    {
        switch (containerSizeInBits())
        {
            case 8:  bufferLPCMU8.resize (size); break;
            case 16: bufferLPCM16.resize (size); break;
            case 24: bufferLPCM24.resize (size); break;
            case 32: bufferLPCM32.resize (size); break;
        }
    }
    else if (isFloatPCM())
    {
        // We don't need to use a buffer when performing *interleaved* reading
        // and source encoding matches destination encoding,
        // i.e. double -> double or float -> float,
        // but at this point, we don't know if that will be the case
        // so we allocate just in case.
        // An alternative approach would be to check if the buffer has been
        // sized each time we read and allocating if not.

        assert (bitDepth() == containerSizeInBits());

        if      (containerSizeInBits() == 32) bufferFloatPCM32.resize (size);
        else if (containerSizeInBits() == 64) bufferFloatPCM64.resize (size);
    }
}

// Calculates the absolute value of the minimum value for a signed integer
// of the given bit depth for use in normalization when decoding to a
// floating-point type, e.g. for 16-bit (signed) linear PCM, the absolute
// value of the minimum value is 32768 and can be used to convert the integer
// value into a normalized float or double through division.
inline double Reader::normalization (uint16_t bitDepth)
{
    assert (bitDepth >= 1 && bitDepth <= 64);
    return static_cast<double> (uint64_t{1} << (bitDepth - 1));
}

} // namespace wav

#endif

