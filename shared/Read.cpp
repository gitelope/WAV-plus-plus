
#include "Read.hpp"
#include "reading_bytes.hpp"
#include "WaveFile.hpp" // consider moving enums and namespaced constants back into a formats.hpp

#include <filesystem>
#include <fstream>
#include <iostream>
#include <cassert>

namespace wav
{

void Read (std::string const & filepath, std::vector<double> & destination, Info & info, bool verbose)
{
    static_assert (std::endian::native == std::endian::little, "Only little endian systems are supported at the moment.");

    if (! std::filesystem::is_regular_file (filepath))
    {
        throw std::runtime_error ("File does not exist: " + filepath);
    }
    if (verbose) std::cout << "\n" << filepath << std::endl;

    std::ifstream in {filepath, std::ios::binary | std::ios::in};

    ReadInfo readInfo;

    ReadRiffChunk (in, readInfo, verbose);

    const long fileLength = std::filesystem::file_size (filepath);
    if (readInfo.riffChunkSize + 8 != fileLength) throw std::runtime_error ("RIFF chunk size + 8 does not equal file size.");
    if (verbose) std::cout << "file length: " << fileLength << std::endl;

    ReadSubchunks (in, destination, info, readInfo, verbose);

    in.close();
}

void ReadRiffChunk (std::ifstream & in, ReadInfo & readInfo, bool verbose)
{
    const auto id {fourCharacterCode (in)};     // RIFF
    readInfo.riffChunkSize = uint32_le (in);    // file length (in bytes) - 8
    const auto format {fourCharacterCode (in)}; // WAVE
    if (id != "RIFF") throw std::runtime_error ("RIFF chunk ID should be \"RIFF\".");
    if (format != "WAVE") throw std::runtime_error ("RIFF chunk format should be \"WAVE\".");
}

void ReadSubchunks (std::ifstream & in, std::vector<double> & destination, Info & info, ReadInfo & readInfo, bool verbose)
{
    if (verbose) std::cout << "\nchunks encountered: " << std::endl;

    while (in.peek() != EOF)
    {
        const auto chunkID {fourCharacterCode (in)};
        if (verbose) std::cout << chunkID;

        if      (chunkID == "fmt ") ReadFormatChunk (in, info, readInfo);
        else if (chunkID == "fact") ReadFactChunk   (in, info);
        else if (chunkID == "data") ReadDataChunk   (in, destination, info, readInfo);
        else
        {
            // skip to next chunk
            const auto size = uint32_le (in);

            if (verbose)
            {
                auto position {in.tellg()};
                position -= static_cast <decltype(position)> (4);
                std::cout << " (skipped - index " << position << ", size " << size << ")";
            }

            in.ignore (size);

            // RIFF sub-chunks (not to be confused with "RIFF" chunks)
            // are not allowed to have an odd number of bytes.
            // If there are an odd number of meaningful bytes, that odd
            // number will be the reported size but there will be an
            // unreported byte of padding at the end to make for an even
            // number. However, all padding should be reflected in the main
            // "RIFF" chunk size.
            in.ignore (size % 2); // When chunk size is odd, there will be one byte of padding.
        }
        if (verbose) std::cout << std::endl;
    }
    if (verbose) std::cout << std::endl;
}

void ReadFormatChunk (std::ifstream & in, Info & info, ReadInfo & readInfo)
{
    // These fields are found in formats WAVEFORMAT, PCMWAVEFORMAT, WAVEFORMATEX, and WAVEFORMATEXTENSIBLE:
    //
    // no need to read ID since it has already been read and has the expected value
    //
    const auto chunkSize {uint32_le (in)};
    // format chunk size
    // WAVEFORMAT:                                                       14
    // PCMWAVEFORMAT:                                             14+2 = 16
    // WAVEFORMATEX:                                            14+2+2 = 18
    // WAVEFORMATEXTENSIBLE (signed int PCM and IEEE float): 14+2+2+22 = 40
    if (chunkSize != 14 && chunkSize != 16 && chunkSize != 18 && chunkSize != 40)
    {
        throw std::runtime_error ("fmt chunk size (" + std::to_string (chunkSize) + ") is unsupported.");
    }
    //
    const auto formatCode {uint16_le (in)};
    if (formatCode != FormatCode::LINEAR_PCM && formatCode != FormatCode::FLOAT_PCM && formatCode != FormatCode::EXTENSIBLE)
    {
        throw std::runtime_error (std::format ("Format not supported: 0x{:04X}", formatCode));
    }
    //
    info.channels = uint16_le (in);
    //
    info.sampleRate = uint32_le (in);
    //
    [[maybe_unused]] const auto byteRate {uint32_le (in)};
    //
    readInfo.blockAlign = uint16_le (in);

    // This field is found in PCMWAVEFORMAT, WAVEFORMATEX, and WAVEFORMATEXTENSIBLE,
    // but must be inferred in WAVEFORMAT.
    //
    // For WAVEFORMAT, bits per sample must be inferred from block align, number of channels,
    // and the assumption that a byte is 8 bits.
    //
    // WAVEFORMATEXTENSIBLE also contains validBitsPerSample, allowing for a standardized way
    // of packing samples into a container of larger size.
    //
    // Apparently, some encoders in the past have packed smaller samples into larger containers
    // in a non-standard way with PCMWAVEFORMAT and possibly WAVEFORMATEX, by using bitsPerSamples
    // as the sample size and 8 * blockAlign / channels as the container size. For this reason,
    // this function assumes that for PCMWAVEFORMAT and WAVEFORMATEX, bitsPerSample is the sample
    // size and 8 * blockAlign / channels is the container size, resulting in dequantization to
    // the intended amplitude and not a smaller one. This shouldn't be a problem for when reading
    // standard PCMWAVEFORMAT and WAVEFORMATEX files since bitsPerSample should equal
    // 8 * blockAlign / channels and thus the container since is interpreted correctly and the
    // dequantization constant is computed correctly.
    //
    // In summary, bitPerSample is herein interpreted as follows:
    // PCMWAVEFORMAT:        sample size
    // WAVEFORMATEX:         sample size
    // WAVEFORMATEXTENSIBLE: container size
    uint16_t bitsPerSample {0};
    if (chunkSize == 16 || chunkSize == 18 || chunkSize == 40)
    {
        bitsPerSample = uint16_le (in);

        if (formatCode == FormatCode::EXTENSIBLE)
        {
            // Since WAVEFORMATEXTENSIBLE has bits_per_sample (container size) AND valid_bits_per_sample (sample size),
            // it makes good sense even for a permissive parser to enforce the following.
            //     1. bits_per_sample must be a multiple of 8
            //     2. bits_per_sample = 8 * block_align / channels
            //        (See large preceding comment about sample size, container size, and non-standard usage.)
            if (bitsPerSample % 8 != 0)
                throw std::runtime_error ("For WAVEFORMATEXTENSIBLE, bits per sample must be a multiple of 8.");
            if (bitsPerSample != 8 * readInfo.blockAlign / info.channels)
                throw std::runtime_error ("For WAVEFORMATEXTENSIBLE, bits per sample must equal 8 * block_align / channels.");
        }
        else if (formatCode == FormatCode::FLOAT_PCM)
        {
            // Assuming for reasonably well-formed float PCM WAVs (of format PCMFORMAT and WAVEFORMATEX):
            //     1. bits_per_sample will equal either 32 or 64
            //     2. bits_per_sample = 8 * block_align / channels
            if (bitsPerSample != 32 && bitsPerSample != 64)
                throw std::runtime_error ("For float PCM, bits per sample must be 32 or 64.");
            if (bitsPerSample != 8 * readInfo.blockAlign / info.channels)
                throw std::runtime_error ("For float PCM, bits per sample must equal 8 * block_align / channels.");
        }

        if ((formatCode == FormatCode::EXTENSIBLE || formatCode == FormatCode::FLOAT_PCM) && bitsPerSample != 8 * readInfo.blockAlign / info.channels)
        {
            throw std::runtime_error ("bits_per_sample should equal 8 * block_align / channels for this format.");
            // This doesn't apply to LINEAR_PCM in order to support non-standard usage in which meaningful
            // bit depth is lower than container size and yet the WAV is not of the format WAVEFORMATEXTENSIBLE.
            // In this scenario, bits_per_sample is made to equal the number of significant bits and is less than
            // 8 * format.block_align / format.channels (the container size). For uncompressed linear and float PCM,
            // (stored in PCMWAVEFORMAT, WAVEFORMATEX, and WAVEFORMATEXTENSIBLE) these should be equal in standard
            // usage.
        }
    }

    // This field is found in WAVEFORMATEX and WAVEFORMATEXTENSIBLE:
    // WAVEFORMATEX should have a format chunk size of 18 and a cbSize value of 0 for signed int PCM and float.
    // WAVEFORMATEXTENSIBLE should have a format chunk size of 40 (18 + 22) and a cbSize of 22.
    if (chunkSize == 18 || chunkSize == 40)
    {
        const auto extraFormatInformationSize {uint16_le (in)}; // a.k.a. cbSize
        if (extraFormatInformationSize != chunkSize - 18)
            throw std::runtime_error ("Invalid cbSize: " + std::to_string (extraFormatInformationSize));
    }

    // These 3 fields are found in WAVEFORMATEXTENSIBLE:
    uint16_t validBitsPerSample {0};
    std::bitset<32> channelMask;
    GUID subFormat;
    if (chunkSize == 40)
    {
        validBitsPerSample = uint16_le (in);
        channelMask = std::bitset<32> (uint32_le (in));
        subFormat = guid (in);

        if (subFormat != ExtensibleFormat::LINEAR_PCM && subFormat != ExtensibleFormat::FLOAT_PCM)
            throw std::runtime_error ("Extensible subformat not supported: " + subFormat.str());
        if (validBitsPerSample > bitsPerSample)
            throw std::runtime_error ("Valid bits per sample (in WAVEFORMATEXTENSIBLE) must not be greater than bits per sample.");
    }

    // set sample bit depth and sample container size in Info
    info.containerSizeInBits = 8 * readInfo.blockAlign / info.channels;
    switch (chunkSize)
    {
        case 14: info.bitDepth = info.containerSizeInBits; break; // WAVEFORMAT - Not likely to encounter but who knows?
        case 16: info.bitDepth = bitsPerSample;            break; // PCMWAVEFORMAT. In standard usage, this is equal to container size.
        case 18: info.bitDepth = bitsPerSample;            break; // WAVEFORMATEX. In standard usage, this is equal to container size.
        case 40: info.bitDepth = validBitsPerSample;       break; // WAVEFORMATEXTENSIBLE
        default: assert (false); // should have been caught earlier
    }

    switch (formatCode)
    {
        case FormatCode::LINEAR_PCM: info.isLinearPCM = true; break;
        case FormatCode::FLOAT_PCM:  info.isFloatPCM  = true; break;
        case FormatCode::EXTENSIBLE:
        {
            if      (subFormat == ExtensibleFormat::LINEAR_PCM) info.isLinearPCM = true;
            else if (subFormat == ExtensibleFormat::FLOAT_PCM)  info.isFloatPCM  = true;
            break;
        }
    }
    if (info.isLinearPCM && info.containerSizeInBits != 8 && info.containerSizeInBits != 16 && info.containerSizeInBits != 24 && info.containerSizeInBits != 32)
    {
        throw std::runtime_error (std::format ("Unsupported linear PCM container size: {}", info.containerSizeInBits));
    }
    if (info.isFloatPCM && ((info.containerSizeInBits != 32 && info.containerSizeInBits != 64) || info.containerSizeInBits != info.bitDepth))
    {
        throw std::runtime_error ("For float PCM, bits per sample must be 32 or 64 and container size must equal sample size.");
    }

    // In the future, if this code is used to read format chunks of odd byte length, we must skip the last padding byte.
}

void ReadFactChunk (std::ifstream & in, Info & info)
{
    const auto chunkSize {uint32_le (in)};
    // "fact" chunk size should be 4 for float PCM (and linear PCM though the chunk is not required)
    if (chunkSize != 4) throw std::runtime_error ("fact chunk size (" + std::to_string (chunkSize) + ") should be 4.");

    // For linear and floar PCM, the "fact" chunk will only provide redundant information–
    // the total number of frames of audio. This should be equal to data chunk size divided by
    // block align.
    const auto frames {uint32_le (in)}; // samples per channel
    // Checking for consistency:
    if (info.frames > 0 && frames != info.frames)
    {
        // If fmt and data chunk have been read, then info.frames should equal
        // data chunk size / block align which should equal the number of frames
        // specified in the fact chunk.
        throw std::runtime_error ("The number of frames in the fact chunk is not equal to the data chunk size divided by block align.");
    }
    else info.frames = frames;
}

void ReadDataChunk (std::ifstream & in, std::vector<double> & destination, Info & info, ReadInfo & readInfo)
{
    // No need to read id since it has already been read and has the expected value.
    const auto chunkSize {uint32_le (in)};
    if (chunkSize % readInfo.blockAlign != 0)
    {
        throw std::runtime_error ("Size of data chunk is not divisible by block_align.");
    }
    info.frames = chunkSize * info.channels / readInfo.blockAlign;

    assert (info.frames > 0);
    assert (info.channels > 0);
    destination.resize (info.frames * info.channels);

    if      (info.isLinearPCM) ReadLinearPCM (in, destination, info);
    else if (info.isFloatPCM)  ReadFloatPCM  (in, destination, info);


    // Padding:
    // If the chunk size is odd, there will be a padding byte of 0x00,
    // e.g. 24-bit samples require 3 bytes and therefore a mono file with seven samples
    // implies a chunk size of 1 x 7 x 3 = 21 bytes,
    // so an extra byte will be appended to the end of the data.
    // After reading the 21 bytes, we must skip the next "unreported" 22nd padding byte.
    // (8-bit and 24-bit mono files with odd sample lengths will need the padding.)
    in.ignore (chunkSize % 2); // When chunk size is odd, increment position by 1.
}

void ReadLinearPCM (std::ifstream & in, std::vector<double> & destination, Info const & info)
{
    // Divide signed integer by the absolute value of the largest negative int for the bit depth.
    // This is equal to one greater than the largest positive value for the bit depth.
    // Example: for 16-bits, the range is [-32768, 32767] so we divide by 32768.
    // This ensures that our float is within [-1.0, 1.0] regardless of how it was clamped.
    // (For what it's worth, it will never equal +1.0.)
    const double two_to_the_n_minus_one = pow (2.0, info.bitDepth - 1);

    switch (info.containerSizeInBits)
    {
        case 8:
            for (auto & sample : destination)
            {
                sample = (uint8 (in) - 128.) / two_to_the_n_minus_one;
            }
            break;

        case 16:
            for (auto & sample : destination)
            {
                sample = int16_le (in) / two_to_the_n_minus_one;
            }
            break;

        case 24:
            for (auto & sample : destination)
                sample = int24_le (in) / two_to_the_n_minus_one;
            break;

        case 32:
            for (auto & sample : destination)
                sample = int32_le (in) / two_to_the_n_minus_one;
            break;
    }
}

void ReadFloatPCM (std::ifstream & in, std::vector<double> & destination, Info const & info)
{
    assert (info.containerSizeInBits == info.bitDepth);

    switch (info.containerSizeInBits)
    {
        case 32:
            for (auto & sample : destination)
                sample = float32_le (in);
            break;

        case 64:
            for (auto & sample : destination)
                sample = float64_le (in);
            break;
    }
}

} // namespace wav
