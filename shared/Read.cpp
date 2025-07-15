
#include "Read.hpp"
#include "ReadingBytes.hpp"
#include "Exceptions.hpp"

#include <filesystem>
#include <fstream>
#include <cassert>

namespace wav
{

std::pair <std::vector<double>, struct Info>
Read (std::string const & filepath,
      size_t requestedBufferSize)
{
    std::vector<double> data;
    Info info;
    Read (filepath, data, info, requestedBufferSize);
    return {std::move (data), info};
}

void Read (std::string const & filepath,
           std::vector<double> & destination,
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
/*
void Read (std::string const & filepath,
           std::vector<std::vector<double>> & destination,
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
*/
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

void ReadData (std::string const & filepath,
               std::vector<double> & destination,
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

    const auto sampleSizeInBytes {static_cast<size_t> (info.containerSizeInBits / 8)};
    const auto totalBytesToRead {numSamples * sampleSizeInBytes};

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
/*
void ReadData (std::string const & filepath,
               std::vector <std::vector<double>> & destination,
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

    destination.resize (info.channels, std::vector<double> (info.frames));

    const auto sampleSizeInBytes {info.containerSizeInBits / 8};

    const auto totalBytesToRead {numSamples * sampleSizeInBytes};

    if (requestedBufferSizeInBytes < sampleSizeInBytes)
    {
        requestedBufferSizeInBytes = sampleSizeInBytes;
    }
    // If requestedBufferSizeInBytes does not correspond to a whole number of samples,
    // lower it to accomodate exactly the largest whole number of samples whose size in bytes
    // is less than requestedBufferSizeInBytes.
    // e.g. When bit depth is 24-bit (3 bytes), 4096 bytes would correspond to 1,365 samples with one byte left over,
    // so we would adjust the requested size to 1,365 * 3 = 4095 bytes.
    const auto adjustedRequestSizeInBytes {(requestedBufferSizeInBytes / sampleSizeInBytes) * sampleSizeInBytes};
    // If the number of bytes to read is smaller than the adjusted buffer size, set buffer size to the former instead.
    std::vector<std::byte> buffer (std::min (totalBytesToRead, adjustedRequestSizeInBytes));

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
*/
void ReadLinearPCM (std::vector<std::byte> const & buffer,
                    std::vector<double> & destination,
                    Info const & info,
                    size_t samplesToRead,
                    size_t writePosition)
{
    // Divide signed integer by the absolute value of the largest negative int for the bit depth.
    // This is equal to one greater than the largest positive value for the bit depth.
    // Example: for 16-bits, the range is [-32768, 32767] so we divide by 32768.
    // This ensures that our float is within [-1.0, 1.0] regardless of how it was clamped.
    // (For what it's worth, it will never equal +1.0.)
    const double two_to_the_n_minus_one = pow (2.0, info.bitDepth - 1);

    size_t pos {0};
    switch (info.containerSizeInBits)
    {
        case 8:
            for (size_t i=0; i<samplesToRead; ++i)
            {
                destination [writePosition + i] = (uint8 (buffer, pos) - 128.) / two_to_the_n_minus_one;
            }
            break;

        case 16:
            for (size_t i=0; i<samplesToRead; ++i)
            {
                destination [writePosition + i] = int16_le (buffer, pos) / two_to_the_n_minus_one;
            }
            break;

        case 24:
            for (size_t i=0; i<samplesToRead; ++i)
                destination [writePosition + i] = int24_le (buffer, pos) / two_to_the_n_minus_one;
            break;

        case 32:
            for (size_t i=0; i<samplesToRead; ++i)
                destination [writePosition + i] = int32_le (buffer, pos) / two_to_the_n_minus_one;
            break;
    }
}

void ReadFloatPCM (std::vector<std::byte> const & buffer,
                   std::vector<double> & destination,
                   Info const & info,
                   size_t samplesToRead,
                   size_t writePosition)
{
    assert (info.containerSizeInBits == info.bitDepth);

    size_t pos {0};
    switch (info.containerSizeInBits)
    {
        case 32:
            for (size_t i=0; i<samplesToRead; ++i)
                destination [writePosition + i] = float32_le (buffer, pos);
            break;

        case 64:
            for (size_t i=0; i<samplesToRead; ++i)
                destination [writePosition + i] = float64_le (buffer, pos);
            break;
    }
}

} // namespace wav

