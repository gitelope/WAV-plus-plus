// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_ChunkInfoImplementation_hpp
#define wavplusplus_ChunkInfoImplementation_hpp

#include "ReadingBytes.hpp"
#include "AudioSummary.hpp"
#include "FileOffset.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <format>
#include <limits>
#include <string_view>
#include <type_traits>

namespace wav
{

inline ChunkInfo::ChunkInfo (std::filesystem::path const & filepath)
{
    auto in = fileForReading (filepath);
    ReadFile (in.get());
}

inline ChunkInfo::ChunkInfo (std::FILE * in)
{
    ReadFile (in);
}

inline std::string ChunkInfo::summary() const
{
    bool isLinear;
    uint16_t bitDepth;
    uint16_t containerSize;

    switch (format.code)
    {
        case FormatCode::LinearPCM:
            isLinear = true;
            bitDepth = format.bitsPerSample;
            containerSize = static_cast<uint16_t> (8 * format.blockAlign / format.channels);
            break;
        case FormatCode::FloatPCM:
            isLinear = false; // must be floating-point
            bitDepth = format.bitsPerSample;
            containerSize = bitDepth;
            assert (containerSize == 8 * format.blockAlign / format.channels);
            break;
        case FormatCode::Extensible:
            if (format.subFormat == ExtensibleFormat::LinearPCM) isLinear = true;
            else
            {
                assert (format.subFormat == ExtensibleFormat::FloatPCM);
                isLinear = false;
            }
            bitDepth = format.validBitsPerSample;
            containerSize = format.bitsPerSample;
            break;
        default:
            assert (false); // should have been caught earlier
            throw ReadError (std::format ("Format not supported: 0x{:04X}. This should have been caught during decoding. Audio may be corrupted.", format.code));
    }

    const auto frames = data.size / format.blockAlign;
    const bool isPadded = (containerSize > bitDepth);

    return audioSummary (isLinear, bitDepth, containerSize, format.sampleRate,
                         format.channels, frames, data.size, isPadded);
}

int peek (std::FILE * in); // check if there's data available
void SkipChunk (std::FILE *, uint32_t);

inline void ChunkInfo::ReadFile (std::FILE * in)
{
    actualFileSize = fileSize (in);

    const auto firstChunkID {fourCharacterCode (in)};
    if (firstChunkID != riff.id) throw ReadError ("RIFF chunk ID not found.");
    ReadRiffChunk (in);
    RegisterChunk (firstChunkID, 0);

    while (peek (in) != EOF)
    {
        const auto chunkStart {tell (in)};
        if (chunkStart == -1) throw ReadError ("Failed to get chunk position.");
        const auto chunkID {fourCharacterCode (in)};
        RegisterChunk (chunkID, chunkStart);

        if      (chunkID == format.id) ReadFormatChunk                 (in);
        else if (chunkID == fact.id)   ReadFactChunk                   (in);
        else if (chunkID == peak.id)   ReadPeakChunk                   (in, chunkStart);
        else if (chunkID == data.id)   ReadDataChunkSizeAndSkipSamples (in);
        else if (chunkID == riff.id)   throw ReadError ("Invalid format: File contains more than one RIFF chunk.");
        else
        {
            // Skip rest of chunk (including any padding byte).
            //
            // RIFF sub-chunks (not to be confused with "RIFF" chunks)
            // are not allowed to have an odd number of bytes.
            // If there are an odd number of meaningful bytes, that odd
            // number will be the reported size but there will be an
            // unreported byte of padding at the end to make for an even
            // number. In contrast, all chunk padding should be reflected
            // in the main "RIFF" chunk size.
            RegisterSkippedChunk (chunkID, chunkStart);
            const auto size {uint32_le (in)};
            // SkipChunk() detects and carefully skips potential padding byte.
            // (Best not to add it in ahead of time since that could result in overflow.)
            SkipChunk (in, size);
        }
    }
    ValidateAfterParsing();
}

inline void SkipChunk (std::FILE * file, uint32_t size)
{
    int64_t skipAmount {static_cast<int64_t> (size) + (size % 2)}; // padding byte if odd
    if (seek (file, skipAmount, SEEK_CUR) != 0)
    {
        throw ReadError ("Failed to skip chunk.");
    }
}

inline int peek (std::FILE * in)
{
    int c = std::fgetc (in);
    if (c != EOF)
    {
        std::ungetc (c, in); // Put the character back
    }
    return c;
}

inline void ChunkInfo::ValidateAfterParsing()
{
    if (!fmtChunkEncountered)
    {
        throw ReadError ("Invalid format: missing fmt chunk.");
    }
    if (!dataChunkEncountered)
    {
        throw ReadError ("Invalid format: missing data chunk.");
    }
}

inline void ChunkInfo::ReadRiffChunk (std::FILE * in)
{
    // Technically, the whole file is a RIFF chunk. This is just the first part of it.
    // No need to read id since it has already been read and has the expected value.
    riff.size = uint32_le (in);
    if (fourCharacterCode (in) != riff.format)
    {
        throw ReadError ("The format in the RIFF chunk should be \"" + riff.format + "\".");
    }
}

inline void ChunkInfo::ReadFormatChunk (std::FILE * in)
{
    if (fmtChunkEncountered)
    {
        throw ReadError (ReadErrorCode::MultipleFmtChunks, "Invalid format: File contains more than one fmt chunk.");
    }

    // WAVEFORMAT, PCMWAVEFORMAT, WAVEFORMATEX, and WAVEFORMATEXTENSIBLE:
    format.size = uint32_le (in);
    if (! isSupportedFormatChunkSize (format.size))
    {
        throw ReadError (ReadErrorCode::UnsupportedFmtChunkSize, "fmt chunk size (" + std::to_string (format.size) + ") is unsupported.");
    }
    format.code = uint16_le (in);
    format.channels = uint16_le (in);
    format.sampleRate = uint32_le (in);
    format.byteRate = uint32_le (in);
    format.blockAlign = uint16_le (in);

    // PCMWAVEFORMAT, WAVEFORMATEX, and WAVEFORMATEXTENSIBLE:
    if (format.size == FormatChunkSize::PCMWAVEFORMAT ||
        format.size == FormatChunkSize::WAVEFORMATEX  ||
        format.size == FormatChunkSize::WAVEFORMATEXTENSIBLE)
    {
        format.bitsPerSample = uint16_le (in);
    }

    // WAVEFORMATEX and WAVEFORMATEXTENSIBLE:
    if (format.size == FormatChunkSize::WAVEFORMATEX ||
        format.size == FormatChunkSize::WAVEFORMATEXTENSIBLE)
    {
        format.cbSize = uint16_le (in);
    }

    // WAVEFORMATEXTENSIBLE:
    if (format.size == FormatChunkSize::WAVEFORMATEXTENSIBLE)
    {
        format.validBitsPerSample = uint16_le (in);
        format.channelMask = uint32_le (in);
        format.subFormat = guid (in);
    }

    // In the future, if this code is used to read format chunks of odd-byte-length,
    // we must skip the last padding byte.
    if (format.size % 2)
    {
        if (seek (in, 1, SEEK_CUR) != 0) throw ReadError ("Failed to skip format chunk padding.");
    }

    ValidateFormatChunk();

    fmtChunkEncountered = true;
}
inline void ChunkInfo::ValidateFormatChunk()
{
    // WAVEFORMAT, PCMWAVEFORMAT, WAVEFORMATEX, and WAVEFORMATEXTENSIBLE
    if (! isSupportedFormatCode (format.code))
    {
        throw ReadError (ReadErrorCode::UnsupportedFormatCode, std::format ("Format not supported: 0x{:04X}", format.code));
    }
    if (format.channels == 0)
    {
        throw ReadError (ReadErrorCode::ChannelsZero, "Invalid value: channels must not be zero.");
    }
    if (format.channels > maxChannels)
    {
        throw ReadError (ReadErrorCode::ChannelsExceedMax, "Invalid value: channels must not be greater than " + std::to_string (static_cast<unsigned> (maxChannels)) + ".");
    }
    if (format.sampleRate == 0)
    {
        throw ReadError (ReadErrorCode::SampleRateZero, "Invalid value: sample rate must not be zero.");
    }
    if (format.blockAlign == 0)
    {
        throw ReadError (ReadErrorCode::BlockAlignZero, "Invalid value: block align must not be zero.");
    }
    if (format.blockAlign % format.channels != 0)
    {
        throw ReadError (ReadErrorCode::BlockAlignNotDivisibleByChannels, "Invalid value: block align must be divisible by channels.");
    }

    // PCMWAVEFORMAT, WAVEFORMATEX, and WAVEFORMATEXTENSIBLE
    if (format.size == FormatChunkSize::PCMWAVEFORMAT      ||
        format.size == FormatChunkSize::WAVEFORMATEX       ||
        format.size == FormatChunkSize::WAVEFORMATEXTENSIBLE)
    {
        if (format.bitsPerSample == 0)
        {
            throw ReadError (ReadErrorCode::BitsPerSampleZero, "Invalid value: bits per sample must be greater than 0.");
        }
        if (format.bitsPerSample > 64)
        {
            throw ReadError (ReadErrorCode::BitsPerSampleExceedsSupportedMax, "Invalid value: bits per sample must be less than or equal to 64.");
        }
        if (format.code == FormatCode::Extensible) // float and integer extensible
        {
            // Since WAVEFORMATEXTENSIBLE has bitsPerSample AND validBitsPerSample,
            // it makes good sense even for a permissive parser to enforce the following:
            //
            //     1. bitsPerSample must be a multiple of 8
            //     2. bitsPerSample = 8 * blockAlign / channels
            //
            // We will check that float extensible bitsPerSample is 32 or 64 later after we read subFormat.
            //
            if (format.bitsPerSample % 8 != 0)
            {
                throw ReadError (ReadErrorCode::ExtensibleBitsPerSampleNotMultipleOf8, "For WAVEFORMATEXTENSIBLE, bits per sample must be a multiple of 8.");
            }
            if (format.bitsPerSample != 8 * format.blockAlign / format.channels)
            {
                throw ReadError (ReadErrorCode::ExtensibleBitsPerSampleChannelsBlockAlignMismatch, "For WAVEFORMATEXTENSIBLE, bits per sample must equal 8 * blockAlign / channels.");
            }
        }
        else if (format.code == FormatCode::FloatPCM) // PCMWAVEFORMAT and WAVEFORMATEX floats
        {
            // Assuming for reasonably well-formed float PCM WAVs (of format PCMWAVEFORMAT and WAVEFORMATEX):
            //
            //     1. bitsPerSample will equal either 32 or 64
            //     2. bitsPerSample = 8 * blockAlign / channels
            //
            if (format.bitsPerSample != 32 && format.bitsPerSample != 64)
            {
                throw ReadError ("For float PCM, bits per sample must be 32 or 64.");
            }
            if (format.bitsPerSample != 8 * format.blockAlign / format.channels)
            {
                throw ReadError (ReadErrorCode::FloatBitsPerSampleChannelsBlockAlignMismatch, "For float PCM, bits per sample must equal 8 * blockAlign / channels.");
            }
        }

        // Note that WAV++ does not require
        // format.bitsPerSample = 8 * format.blockAlign / channels
        // for PCMWAVEFORMAT LPCM and WAVEFORMATEX LPCM in order to support
        // common non-standard usage in which a lower bit depth is packed into
        // a larger container.
        //
        // In this scenario, bitsPerSample is interpreted as the number of significant
        // (or "valid") bits and is less than 8 * format.blockAlign / format.channels,
        // which is, as usual, the container size. (For integer and float PCM stored in
        // PCMWAVEFORMAT, WAVEFORMATEX, and WAVEFORMATEXTENSIBLE these are expected to
        // be equal in standard usage.)
        //
        // The standard way of accomplishing bit packing is to use WAVEFORMATEXTENSIBLE's
        // additional validBitsPerSample as the actual bit depth and bitsPerSample as the
        // container size. Note that the non-standard bit-packing method described above
        // treats bitsPerSample not as the container size but as the effective bit depth,
        // flipping the role it plays.
    }

    // WAVEFORMATEX and WAVEFORMATEXTENSIBLE
    //
    // WAVEFORMATEX should have a format chunk size of 18 and a cbSize value of 0 for linear and float PCM.
    // WAVEFORMATEXTENSIBLE should have a format chunk size of 40 (18 + 22) and a cbSize of 22.
    if (format.size == FormatChunkSize::WAVEFORMATEX      ||
        format.size == FormatChunkSize::WAVEFORMATEXTENSIBLE)
    {
        if (format.cbSize != format.size - FormatChunkSize::WAVEFORMATEX)
        {
            throw ReadError ("Invalid cbSize: " + std::to_string (static_cast<unsigned int> (format.cbSize)));
        }
    }

    // WAVEFORMATEXTENSIBLE only
    if (format.size == FormatChunkSize::WAVEFORMATEXTENSIBLE)
    {
        if (format.validBitsPerSample == 0 || format.validBitsPerSample > 64)
        {
            throw ReadError ("Invalid value: valid bits per sample must be greater than 0 and less than or equal to 64.");
        }
        if (format.subFormat != ExtensibleFormat::LinearPCM && format.subFormat != ExtensibleFormat::FloatPCM)
        {
            throw ReadError ("Extensible subformat not supported: " + format.subFormat.str());
        }
        if (format.subFormat == ExtensibleFormat::FloatPCM
            && format.bitsPerSample != 32 && format.bitsPerSample != 64)
        {
            throw ReadError ("For float PCM, bits per sample must be 32 or 64.");
        }
    }

    const auto containerSize {8u * format.blockAlign / format.channels};

    if ((format.code == FormatCode::LinearPCM ||
         (format.code == FormatCode::Extensible && format.subFormat == ExtensibleFormat::LinearPCM))
        && containerSize != 8u && containerSize != 16u && containerSize != 24u && containerSize != 32u)
    {
        throw ReadError (std::format ("Unsupported linear PCM container size: {}", containerSize));
    }

    // When bit-packing (standard or non-standard) is used with LPCM,
    // check that effective bit depth <= container size:
    //
    // Standard bit-packing using WAVEFORMATEXTENSIBLE
    // validBitsPerSample must be <= bitsPerSample:
    if (format.size == FormatChunkSize::WAVEFORMATEXTENSIBLE && format.validBitsPerSample > format.bitsPerSample)
    {
        throw ReadError (std::format ("Inconsistent values: WAVEFORMATEXTENSIBLE's validBitsPerSample ({}) > bitsPerSample ({}).", format.validBitsPerSample, format.bitsPerSample));
    }
    // Non-standard bit packing supported in PCMWAVEFORMAT and WAVEFORMATEX
    // bitsPerSample is interpreted as effective (or "valid") bit depth
    // containerSize is derived from blockAlign and channels
    // The former must be <= the latter:
    if (format.code == FormatCode::LinearPCM
        && (format.size == FormatChunkSize::PCMWAVEFORMAT || format.size == FormatChunkSize::WAVEFORMATEX)
        && format.bitsPerSample > containerSize)
    {
        throw ReadError (std::format ("Inconsistent values: bitsPerSample ({}) > containerSize ({})", format.bitsPerSample, containerSize));
    }
}

inline void ChunkInfo::ReadFactChunk (std::FILE * in)
{
    if (factChunkEncountered) throw ReadError ("Invalid format: File contains more than one fact chunk.");
    factChunkEncountered = true;

    const auto size = uint32_le (in);
    if (size != 4) throw ReadError ("Invalid value: fact chunk size must be 4.");
    fact.size = size;

    fact.fileSize = uint32_le (in);
    fact.present = true;
}

inline void ChunkInfo::ReadPeakChunk (std::FILE * in, std::int64_t chunkStart)
{
    // Throws ReadError when:
    //     more than one PEAK
    //     PEAK before fmt
    //     declared size exceeds bytes remaining in file
    //     declared size is less than 4
    //     version is 1 and declared size does not match 4 + 4 + 8 * channels
    //
    // Skips remaining chunk body when:
    //     version is not 1

    if (peakChunkEncountered) throw ReadError ("Invalid format: File contains more than one PEAK chunk.");
    peakChunkEncountered = true;

    if (!fmtChunkEncountered) throw ReadError ("Invalid format: PEAK chunk comes before fmt chunk.");

    peak.size = uint32_le (in);

    // make sure size doesn't imply chunk overruns EOF
    const auto bodyStart = tell (in);
    if (bodyStart == -1)
    {
        throw ReadError ("Failed to get current file position before parsing PEAK chunk body.");
    }
    const auto bytesRemaining {actualFileSize - bodyStart};
    if (peak.size > bytesRemaining)
    {
        throw ReadError (std::format ("Inconsistent value: declared PEAK chunk size ({}) exceeds bytes remaining in the file ({}).", peak.size, bytesRemaining));
    }

    if (peak.size < 4u) throw ReadError ("Invalid value: PEAK chunk size is too small to contain a version field.");

    peak.version = uint32_le (in);

    // Skip any PEAK chunks that aren't v1:
    if (peak.version != 1)
    {
        RegisterSkippedChunk (peak.id, chunkStart);
        SkipChunk (in, peak.size - 4u);
        return;
    }

    // Make sure size matches version 1 expectations given number of channels:
    // expected size = 4 bytes (version) + 4 bytes (timestamp) + 8 bytes per peak (one per channel).
    const uint32_t expectedChunkSize {4u + 4u + 8u * format.channels};
    if (peak.size != expectedChunkSize)
    {
        throw ReadError (std::format ("Invalid value: PEAK chunk size ({}) does not match channel count. Expected: 8 bytes + 8 bytes per channel * {} channels = {} bytes", peak.size, format.channels, expectedChunkSize));
    }

    peak.timestamp = uint32_le (in);

    const auto numPeaks {(peak.size - 8u) / 8u};
    peak.peaks.resize (numPeaks);
    for (auto & p : peak.peaks)
    {
        p.value = float32_le (in);
        p.positionInFrames = uint32_le (in);
    }

    peak.present = true; // present and parsed successfully
}

inline void ChunkInfo::ReadDataChunkSizeAndSkipSamples (std::FILE * in)
{
    if (!fmtChunkEncountered) throw ReadError ("Invalid format: data chunk reached before format chunk.");
    if (dataChunkEncountered) throw ReadError ("Invalid format: File contains more than one data chunk.");

    // No need to read id since it has already been read and has the expected value.

    data.size = uint32_le (in);

    if (data.size % format.blockAlign != 0)
    {
        throw ReadError ("Inconsistent value: size of data chunk is not divisible by block align.");
    }

    // make sure size doesn't imply chunk overuns EOF
    data.offset = tell (in);
    if (data.offset == -1) throw ReadError ("Failed to get current file position for data chunk.");
    const auto bytesRemaining {actualFileSize - data.offset};
    if (data.size > bytesRemaining)
    {
        throw ReadError (std::format ("Inconsistent value: declared data chunk size ({}) exceeds bytes remaining in the file ({}).",
            data.size, bytesRemaining));
    }

    // SkipChunk() detects and carefully skips potential padding byte.
    // (Best not to add it in ahead of time since that could result in overflow.)
    //
    // An odd data chunk size will most likely be the result of writing mono,
    // linear PCM 8-bit or 24-bit WAV, with an odd number of samples,
    // but will also occur with a larger odd number of channels:
    // odd channels x odd sample (container) size in bytes x odd number of samples = odd number of bytes
    SkipChunk (in, data.size);

    dataChunkEncountered = true;
}

inline void ChunkInfo::RegisterChunk (std::string const & id, std::int64_t position)
{
    chunksEncountered.emplace_back (id + " " + std::to_string (position));
}

inline void ChunkInfo::RegisterSkippedChunk (std::string const & id, std::int64_t position)
{
    chunksSkipped.emplace_back (id + " " + std::to_string (position));
}

inline std::ostream & operator<< (std::ostream & os, ChunkInfo const & info)
{
    os << "chunks present\n"
    << "--------------\n";
    for (auto const & c : info.chunksEncountered) os << c << '\n';
    if (info.chunksSkipped.size() > 0)
    {
        os << "chunks skipped: " << '\n';
        for (auto const & c : info.chunksSkipped)
            os << c << '\n';
    }
    os << '\n' << info.riff << "\n\n" << info.format << "\n\n";
    if (info.fact.present)
        os << info.fact << "\n\n";
    if (info.peak.present)
        os << info.peak << "\n\n";
    os << info.data << '\n';

    return os;
}

inline std::ostream & operator<< (std::ostream & os, RiffChunk const & riff)
{
    return os
    << "RIFF chunk" << '\n'
    << "----------" << '\n'
    << "id: " << riff.id << '\n'
    << "chunk size: " << riff.size << " (= file size in bytes minus 8)" << '\n'
    << "format: " << riff.format;
}

inline std::ostream & operator<< (std::ostream & os, FormatChunk const & format)
{
    auto description = [] (uint16_t formatCode)
    {
        switch (formatCode)
        {
            case FormatCode::LinearPCM: return "linear PCM";
            case FormatCode::FloatPCM: return "IEEE float";
            case FormatCode::Extensible: return "extensible";
            default: return "unknown";
        }
    };
    auto fileFormat = [] (uint32_t formatChunkSize)
    {
        switch (formatChunkSize)
        {
            case FormatChunkSize::WAVEFORMAT:           return "WAVEFORMAT";
            case FormatChunkSize::PCMWAVEFORMAT:        return "PCMWAVEFORMAT";
            case FormatChunkSize::WAVEFORMATEX:         return "WAVEFORMATEX";
            case FormatChunkSize::WAVEFORMATEXTENSIBLE: return "WAVEFORMATEXTENSIBLE";
            default: return "unknown";
        }
    };

    os
    << "fmt  chunk" << '\n'
    << "----------" << '\n'
    // WAVEFORMAT, PCMWAVEFORMAT, WAVEFORMATEX, WAVEFORMATEXTENSIBLE
    << "format id: "  << format.id << '\n'
    << std::format ("format chunk size: {} ({})", format.size, fileFormat (format.size)) << '\n'
    << std::format ("format: 0x{:04X} ({})", format.code, description (format.code)) << '\n'
    << "channels: " << format.channels << '\n'
    << "sample rate: " << format.sampleRate << '\n'
    << "byte rate: " << format.byteRate << '\n'
    << "block align: " << format.blockAlign;
    // PCMWAVEFORMAT, WAVEFORMATEX, WAVEFORMATEXTENSIBLE
    if (format.size == FormatChunkSize::PCMWAVEFORMAT      ||
        format.size == FormatChunkSize::WAVEFORMATEX       ||
        format.size == FormatChunkSize::WAVEFORMATEXTENSIBLE)
    {
        os << '\n'
        << "bits per sample: " << format.bitsPerSample;
    }
    // WAVEFORMATEX, WAVEFORMATEXTENSIBLE
    if (format.size == FormatChunkSize::WAVEFORMATEX       ||
        format.size == FormatChunkSize::WAVEFORMATEXTENSIBLE)
    {
        os << '\n'
        << "extra format information size (cbSize): " << format.cbSize;
    }
    // WAVEFORMATEXTENSIBLE
    if (format.size == FormatChunkSize::WAVEFORMATEXTENSIBLE)
    {
        os << '\n'
        << "valid bits per sample: " << format.validBitsPerSample << '\n'
        << "channel mask: " << std::format ("0x{:08X}", format.channelMask) << '\n'
        // << std::format ("{:032b}",  format.channelMask) << '\n' \\ binary
        << format.channelString() << '\n'
        << "sub format GUID: " << format.subFormat << " (";
        if (format.subFormat == ExtensibleFormat::LinearPCM)
        {
            os << "linear PCM";
        }
        else
        {
            assert (format.subFormat == ExtensibleFormat::FloatPCM);
            os << "float PCM";
        }
        os << ")";
    }

    return os;
}

inline std::ostream & operator<< (std::ostream & os, DataChunk const & data)
{
    return os
    << "data chunk" << '\n'
    << "----------" << '\n'
    << "id: " << data.id << '\n'
    << "chunk size: " << data.size << '\n'
    << "[DATA]";
}

inline std::ostream & operator<< (std::ostream & os, FactChunk const & fact)
{
    return os
    << "fact chunk" << '\n'
    << "----------" << '\n'
    << "id: " << fact.id << '\n'
    << "chunk size: " << fact.size << '\n'
    << "file size: " << fact.fileSize;
}

inline std::ostream & operator<< (std::ostream & os, PeakChunk const & peak)
{
    os
    << "PEAK chunk" << '\n'
    << "----------" << '\n'
    << "id: " << peak.id << '\n'
    << "chunk size: " << peak.size << '\n'
    << "version: " << peak.version  << '\n'
    << "timestamp: " << peak.timestamp << '\n'
    << "peaks: ";

    int channel = 0;
    for (auto const & p : peak.peaks)
    {
        os << "\nchannel " + std::to_string (channel++) + ", frame " + std::to_string (p.positionInFrames) + ": " + std::to_string (p.value);
    }

    return os;
}

/// Provides channel information based on number of channels in file and channel mask.
inline std::string FormatChunk::channelString() const
{
    /*
     number of bits set in channel mask vs. number of channels
     ---------------------------------------------------------

     1. channel mask bits set == num channels:

     Display all speaker positions with their channel number.

     Example:
     channel mask: 00000000000000000000000000000011
     [0] front left, [1] front right


     2. channel mask bits set < num channels:

     Display all speaker positions with their channel number and
     then display "unassigned" for each subsequent channel.

     Example:
     channel mask: 00000000000000000000000000110000
     [0] back left, [1] back right, [2] unassigned, [3] unassigned


     3. channel mask bits set > num channels:

     Display all speaker positions with valid audio channel numbers
     and then list speaker positions specified in mask which have no corresponding audio channel.

     Example:
     channel mask: 00000000000000001001001000010000
     [0] back left, [1] side left
     No audio found for position(s) set in channel mask: top front left, top back left


     Note: Channel number is channel's position in the data interleaving sequence, not channel mask index.
     */

    constexpr std::array<std::string_view,18> speakerPositionsNames
    {
        "front left",             //     0x1   0
        "front right",            //     0x2   1
        "front center",           //     0x4   2
        "low frequency",          //     0x8   3
        "back left",              //    0x10   4
        "back right",             //    0x20   5
        "front left of center",   //    0x40   6
        "front right of center",  //    0x80   7
        "back center",            //   0x100   8
        "side left",              //   0x200   9
        "side right",             //   0x400  10
        "top center",             //   0x800  11
        "top front left",         //  0x1000  12
        "top front center",       //  0x2000  13
        "top front right",        //  0x4000  14
        "top back left",          //  0x8000  15
        "top back center",        // 0x10000  16
        "top back right"          // 0x20000  17
    };

    std::string result;

    int channel {0};
    size_t speaker_index {0};
    int positions_assigned {0};
    bool first_element {true};

    // Assign speaker position strings to channels until we either
    // run out of channels or run out of speaker positions set in the channel mask.
    for (; channel < channels && positions_assigned < std::popcount (channelMask); ++speaker_index)
    {
        if ((channelMask >> speaker_index) & 1u)
        {
            ++positions_assigned;
            if (first_element) first_element = false;
            else result += ", ";
            result += "[" + std::to_string (channel++) + "] " + std::string (speakerPositionsNames [speaker_index]);
        }
    }

    // Declare any channels without assigned speaker positions as "unassigned".
    for (; channel < channels; ++channel)
    {
        if (first_element) first_element = false;
        else result += ", ";
        result += "[" + std::to_string (channel) + "] unassigned";
    }

    // Add warning for unused speaker positions when more bits are set
    // in the channel mask than there are actual channels of audio.
    first_element = true;

    for (; positions_assigned < std::popcount (channelMask); ++speaker_index)
    {
        if ((channelMask >> speaker_index) & 1u)
        {
            ++positions_assigned;
            if (first_element)
            {
                result += "\nNo audio found for position(s) set in channel mask: ";
                first_element = false;
            }
            else
            {
                result += ", ";
            }
            result += speakerPositionsNames [speaker_index];
        }
    }

    return result;
}

} // namespace wav

#endif

