
#include "Read.hpp"
#include "ReadingBytes.hpp"
#include "Exceptions.hpp"

#include <filesystem>
#include <fstream>
#include <cassert>

namespace wav
{

// Forward declarations for helper functions:
template <typename FloatType> void ReadData
 (std::string const &, std::vector<FloatType> &, Info const &, DataChunk const &, size_t);
template <typename FloatType> void ReadData
 (std::string const &, std::vector<std::vector<FloatType>> &, Info const &, DataChunk const &, size_t);
template <typename FloatType> void ReadLinearPCM
 (std::vector<std::byte> const &, std::vector<FloatType> &, Info const &, size_t, size_t);
template <typename FloatType> void ReadLinearPCM
 (std::vector<std::byte> const &, std::vector<std::vector<FloatType>> &, Info const &, size_t, size_t);
template <typename FloatType> void ReadFloatPCM
 (std::vector<std::byte> const &, std::vector<FloatType> &, Info const &, size_t, size_t);
template <typename FloatType> void ReadFloatPCM
 (std::vector<std::byte> const &, std::vector<std::vector<FloatType>> &, Info const &, size_t, size_t);
////////////////////////////////////////////

template <typename FloatType>
std::pair <std::vector<FloatType>, struct Info>
ReadInterleaved (std::string const & filepath,
                 size_t requestedBufferSize)
{
    std::vector<FloatType> data;
    Info info;
    Read (filepath, data, info, requestedBufferSize);
    return {std::move (data), info};
}
// explicit instantiation required for linking when separating declarations and definitions
template std::pair <std::vector<float>,  struct Info> ReadInterleaved (std::string const &, size_t);
template std::pair <std::vector<double>, struct Info> ReadInterleaved (std::string const &, size_t);

// De-interleaved version
template <typename FloatType>
std::pair <std::vector<std::vector<FloatType>>, struct Info>
Read (std::string const & filepath,
      size_t requestedBufferSize)
{
    std::vector<std::vector<FloatType>> data;
    Info info;
    Read (filepath, data, info, requestedBufferSize);
    return {std::move (data), info};
}
// explicit instantiation required for linking when separating declarations and definitions
template std::pair <std::vector<std::vector<float>>,  struct Info> Read (std::string const &, size_t);
template std::pair <std::vector<std::vector<double>>, struct Info> Read (std::string const &, size_t);

template <typename FloatType>
void Read (std::string const & filepath,
           std::vector<FloatType> & destination,
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
// explicit instantiation required for linking when separating declarations and definitions
template void Read (std::string const &, std::vector<float> &,  struct Info &, size_t);
template void Read (std::string const &, std::vector<double> &, struct Info &, size_t);

template <typename FloatType>
void Read (std::string const & filepath,
           std::vector<std::vector<FloatType>> & destination,
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
// explicit instantiation required for linking when separating declarations and definitions
template void Read (std::string const &, std::vector<std::vector<float>> &, struct Info &, size_t);
template void Read (std::string const &, std::vector<std::vector<double>> &, struct Info &, size_t);

void Info::ExtractFrom (ChunkInfo const & chunkInfo)
{
    channels = chunkInfo.format.channels;
    sampleRate = chunkInfo.format.sampleRate;
    frames = chunkInfo.data.size / chunkInfo.format.blockAlign;

    // SAMPLE SIZE AND SAMPLE CONTAINER SIZE
    //
    // According to the non-formally defined WAV standard, WAVEFORMATEXTENSIBLE
    // is the only format to be used to pack linear PCM samples into larger
    // containers. bitsPerSample represents the container size while
    // validBitsPerSample represents the number of meaningful bits, a.k.a the
    // sample bit depth.
    //
    // While PCMWAVEFORMAT and WAVEFORMATEX contain bitsPerSample, they do not
    // have a validBitsPerSample. However, due to the fact that bitsPerSample
    // is redundant for linear PCM audio
    // –it can be calculated as 8 * blockAlign / channels–
    // it is possible to pack linear PCM samples into larger containers by
    // interpreting bitsPerSample as the number of valid bits per sample and
    // 8 * blockAlign / channels as the container size.
    //
    // (WAVEFORMAT, unlikely to be enountered, does not have a bitsPerSample and
    // thus the container size and the number of valid bits are one and the same
    // and calculated as 8 * blockAlign / channels.)
    //
    // The following scheme shows how WAV++ determines sample size and sample
    // container size in order to correctly parse both WAV files following the
    // standard and WAV files encoded using the non-standard packing method.
    //
    // format                 container size               sample size
    //
    // WAVEFORMAT             8 * blockAlign / channels    8 * blockAlign / channels
    // PCMWAVEFORMAT          8 * blockAlign / channels    bitsPerSample
    // WAVEFORMATEX           8 * blockAlign / channels    bitsPerSample
    // WAVEFORMATEXTENSIBLE   bitsPerSample                validBitsPerSample

    switch (chunkInfo.format.size)
    {
        case FormatChunkSize::WAVEFORMAT:
            containerSizeInBits = 8 * chunkInfo.format.blockAlign / channels;
            bitDepth = containerSizeInBits;
            break;

        case FormatChunkSize::PCMWAVEFORMAT:
            containerSizeInBits = 8 * chunkInfo.format.blockAlign / channels;
            bitDepth = chunkInfo.format.bitsPerSample;
            break;

        case FormatChunkSize::WAVEFORMATEX:
            containerSizeInBits = 8 * chunkInfo.format.blockAlign / channels;
            bitDepth = chunkInfo.format.bitsPerSample;
            break;

        case FormatChunkSize::WAVEFORMATEXTENSIBLE:
            containerSizeInBits = chunkInfo.format.bitsPerSample;
            bitDepth = chunkInfo.format.validBitsPerSample;
            // Note: FWIW containerSizeInBits should also be equal 8 * chunkInfo.format.blockAlign / channels.
            break;

        default: assert (false); // should have been caught earlier
    }
    switch (chunkInfo.format.code)
    {
        case FormatCode::LINEAR_PCM: isLinearPCM = true; break;
        case FormatCode::FLOAT_PCM:  isFloatPCM  = true; break;
        case FormatCode::EXTENSIBLE:
        {
            if      (chunkInfo.format.subFormat == ExtensibleFormat::LINEAR_PCM) isLinearPCM = true;
            else if (chunkInfo.format.subFormat == ExtensibleFormat::FLOAT_PCM)  isFloatPCM  = true;
            break;
        }
    }
}

std::ostream & operator<< (std::ostream & os, Info const & info)
{
    os << "WAVE ";
    if      (info.isLinearPCM) os << std::format ("{}-bit linear PCM", info.bitDepth);
    else if (info.isFloatPCM)  os << std::format ("{}-bit float PCM",  info.bitDepth);
    else assert (false);
    os << std::format (" / {} kHz / ", info.sampleRate / 1000);
    switch (info.channels)
    {
        case 1:  os << "mono";   break;
        case 2:  os << "stereo"; break;
        default: os << std::format("{} channels", info.channels); break;
    }
    os << std::format (" / {} seconds / {} frames / ", static_cast<double>(info.frames) / info.sampleRate, info.frames);

    const auto bytes = static_cast<long long>(info.frames) * info.channels * info.bitDepth / 8;
    if (bytes >= 1e9) os << std::format ("{:.1f} GB\n", bytes / static_cast<double> (1e9));
    else if (bytes >= 1e6) os << std::format ("{:.1f} MB\n", bytes / static_cast<double> (1e6));
    else if (bytes >= 1e3) os << std::format ("{:.1f} kB\n", bytes / static_cast<double> (1e3));
    else os << bytes << " bytes\n";

    return os;
}

template <typename FloatType>
void ReadData (std::string const & filepath,
               std::vector<FloatType> & destination,
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

template <typename FloatType>
void ReadData (std::string const & filepath,
               std::vector <std::vector<FloatType>> & destination,
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

    destination.resize (info.channels, std::vector<FloatType> (info.frames));

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

template <typename FloatType>
void ReadLinearPCM (std::vector<std::byte> const & buffer,
                    std::vector<FloatType> & destination,
                    Info const & info,
                    size_t samplesToRead,
                    size_t writePosition)
{
    // Normalize signed integers to [-1.0, 1.0) by dividing by 2^(bits-1).
    // Example:
    // 16-bit range: [-32768, 32767], divided by 32768: [-1.0, 0.9999694824]
    const auto two_to_the_n_minus_one  {static_cast<FloatType> (1 << (info.bitDepth - 1))}; // 2^(bits-1)

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

template <typename FloatType>
void ReadLinearPCM (std::vector<std::byte> const & buffer,
                    std::vector<std::vector<FloatType>> & destination,
                    Info const & info,
                    size_t framesToRead,
                    size_t writePosition)
{
    // Normalize signed integers to [-1.0, 1.0) by dividing by 2^(bits-1).
    // Example:
    // 16-bit range: [-32768, 32767], divided by 32768: [-1.0, 0.9999694824]
    const auto two_to_the_n_minus_one  {static_cast<FloatType> (1 << (info.bitDepth - 1))}; // 2^(bits-1)

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

template <typename FloatType>
void ReadFloatPCM (std::vector<std::byte> const & buffer,
                   std::vector<FloatType> & destination,
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

template <typename FloatType>
void ReadFloatPCM (std::vector<std::byte> const & buffer,
                   std::vector<std::vector<FloatType>> & destination,
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

} // namespace wav

