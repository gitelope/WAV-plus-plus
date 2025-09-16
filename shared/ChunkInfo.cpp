
#include "ChunkInfo.hpp"
#include "Exceptions.hpp"
#include <cassert>
#include <sstream>
#include <filesystem>
#include <fstream>

namespace wav
{

ChunkInfo::ChunkInfo (std::string const & filepath)
{
    auto in = fileForReading (filepath);
    if (!in) throw ReadError ("Failed to open: " + filepath);
    ReadFile (in.get());
    Close (in);
}

ChunkInfo::ChunkInfo (FILE * in)
{
    ReadFile (in);
}

void ChunkInfo::ReadFile (FILE * in)
{
    const auto firstChunkID {fourCharacterCode (in)};
    if (firstChunkID != riff.id) throw ReadError ("RIFF chunk ID not found.");
    ReadRiffChunk (in);
    RegisterChunk (firstChunkID, 0);

    while (peek (in) != EOF)
    {
        const auto chunkStart {ftell (in)};
        const auto chunkID {fourCharacterCode (in)};
        RegisterChunk (chunkID, chunkStart);

        if      (chunkID == format.id) ReadFormatChunk       (in);
        else if (chunkID == fact.id)   ReadFactChunk         (in);
        else if (chunkID == peak.id)   ReadPeakChunk         (in);
        else if (chunkID == data.id)   ReadDataChunkInfoOnly (in);
        else
        {
            // Skip to next chunk:
            RegisterSkippedChunk (chunkID, chunkStart);
            const auto size = uint32_le (in);
            // Skip chunk and any padding byte if present.
            //
            // RIFF sub-chunks (not to be confused with "RIFF" chunks)
            // are not allowed to have an odd number of bytes.
            // If there are an odd number of meaningful bytes, that odd
            // number will be the reported size but there will be an
            // unreported byte of padding at the end to make for an even
            // number. In contrast, all chunk padding should be reflected
            // in the main "RIFF" chunk size.
            fseek (in, size + (size % 2), SEEK_CUR);
            if (ferror (in)) throw ReadError ("Failed to skip chunk: " + chunkID);
        }
    }
}

std::string ChunkInfo::summary() const
{
    std::string channelString;
    switch (format.channels)
    {
        case 1: channelString = "mono"; break;
        case 2: channelString = "stereo"; break;
        default: channelString = std::to_string (format.channels) + " channels"; break;
    }

    const auto frames = data.size / format.blockAlign;
    const auto duration = frames / static_cast<double>(format.sampleRate);

    std::string format_string;

    switch (format.code)
    {
        case FormatCode::LinearPCM:
        {
            format_string = std::to_string (format.bitsPerSample) + "-bit linear PCM";
            const int container_size_in_bits = 8 * format.blockAlign / format.channels;
            if (format.bitsPerSample < container_size_in_bits)
                format_string += std::format (" / {}-bit container", container_size_in_bits);
            break;
        }
        case FormatCode::FloatPCM:
        {
            format_string = std::to_string (format.bitsPerSample) + "-bit float PCM";
            break;
        }
        case FormatCode::Extensible:
        {
            format_string = "extensible ";
            format_string += std::to_string (format.validBitsPerSample) + "-bit ";

            if (format.subFormat == ExtensibleFormat::LinearPCM) format_string += "linear PCM";
            else if (format.subFormat == ExtensibleFormat::FloatPCM) format_string += "float PCM";
            else assert (false);

            if (format.validBitsPerSample < format.bitsPerSample)
                format_string += std::format (" / {}-bit container", format.bitsPerSample);
            break;
        }
        default:
            format_string = "unsupported format";
    }
    std::ostringstream oss;

    oss << riff.format << " "
    << format_string << " / "
    << format.sampleRate / 1000 << " kHz / "
    << channelString << " / "
    << frames << " frames" << " / "
    << duration << " seconds" << " / "
    << data.size << " bytes of audio" << std::endl;

    return oss.str();
}

void ChunkInfo::ReadRiffChunk (FILE * in)
{
    // Technically, the whole file is a RIFF chunk. This is just the first part of it.
    // No need to read id since it has already been read and has the expected value.
    riff.size = uint32_le (in);
    if (fourCharacterCode (in) != riff.format)
    {
        throw ReadError ("The format in the RIFF chunk should be \"" + riff.format + "\".");
    }
}

void ChunkInfo::ReadFormatChunk (FILE * in)
{
    // These fields are found in formats WAVEFORMAT, PCMWAVEFORMAT, WAVEFORMATEX, and WAVEFORMATEXTENSIBLE:
    //
    // no need to read ID since it has already been read and has the expected value
    //
    format.size = uint32_le (in);
    if (! isValidFormatChunkSize (format.size))
    {
        throw ReadError ("fmt chunk size (" + std::to_string (format.size) + ") is unsupported.");
    }
    format.code = uint16_le (in);
    if (! isValidFormatCode (format.code))
    {
        throw ReadError (std::format ("Format not supported: 0x{:04X}", format.code));
    }
    format.channels = uint16_le (in);
    format.sampleRate = uint32_le (in);
    format.byteRate = uint32_le (in);
    format.blockAlign = uint16_le (in);

    // This field is found in PCMWAVEFORMAT, WAVEFORMATEX, and WAVEFORMATEXTENSIBLE:
    if (format.size == FormatChunkSize::PCMWAVEFORMAT      ||
        format.size == FormatChunkSize::WAVEFORMATEX       ||
        format.size == FormatChunkSize::WAVEFORMATEXTENSIBLE)
    {
        format.bitsPerSample = uint16_le (in);

        if (format.code == FormatCode::Extensible)
        {
            // Since WAVEFORMATEXTENSIBLE has bits_per_sample AND valid_bits_per_sample,
            // it makes good sense even for a permissive parser to enforce the following.
            //     1. bits_per_sample must be a multiple of 8
            //     2. bits_per_sample = 8 * block_align / channels
            if (format.bitsPerSample % 8 != 0)
            {
                throw ReadError ("For WAVEFORMATEXTENSIBLE, bits per sample must be a multiple of 8.");
            }
            if (format.bitsPerSample != 8 * format.blockAlign / format.channels)
            {
                throw ReadError ("For WAVEFORMATEXTENSIBLE, bits per sample must equal 8 * blockAlign / channels.");
            }
        }
        else if (format.code == FormatCode::FloatPCM)
        {
            // Assuming for reasonably well-formed float PCM WAVs (of format PCMFORMAT and WAVEFORMATEX):
            //     1. bits_per_sample will equal either 32 or 64
            //     2. bits_per_sample = 8 * block_align / channels
            if (format.bitsPerSample != 32 && format.bitsPerSample != 64)
            {
                throw ReadError ("For float PCM, bits per sample must be 32 or 64.");
            }
            if (format.bitsPerSample != 8 * format.blockAlign / format.channels)
            {
                throw ReadError ("For float PCM, bits per sample must equal 8 * blockAlign / channels.");
            }
        }

        if ((format.code == FormatCode::Extensible || format.code == FormatCode::FloatPCM)
            && format.bitsPerSample != 8 * format.blockAlign / format.channels)
        {
            throw ReadError ("bits_per_sample should equal 8 * block_align / channels for this format.");
            // This doesn't apply to LINEAR_PCM in order to support non-standard usage in which
            // meaningful bit depth is lower than container size and yet the WAV is not of the
            // format WAVEFORMATEXTENSIBLE.
            // In this scenario, bits_per_sample is made to equal the number of significant bits
            // and is less than 8 * format.blockAlign / format.channels, the container size. For
            // linear and float PCM, (stored in PCMWAVEFORMAT, WAVEFORMATEX, and
            // WAVEFORMATEXTENSIBLE) these are equal in standard usage.
        }
    }

    // This field is found in WAVEFORMATEX and WAVEFORMATEXTENSIBLE:
    // WAVEFORMATEX should have a format chunk size of 18 and a cbSize value of 0 for linear and float PCM.
    // WAVEFORMATEXTENSIBLE should have a format chunk size of 40 (18 + 22) and a cbSize of 22.
    if (format.size == FormatChunkSize::WAVEFORMATEX || format.size == FormatChunkSize::WAVEFORMATEXTENSIBLE)
    {
        format.extraFormatInformationSize = uint16_le (in); // a.k.a. cbSize
        if (format.extraFormatInformationSize != format.size - FormatChunkSize::WAVEFORMATEX)
        {
            throw ReadError ("Invalid cbSize: " + std::to_string (format.extraFormatInformationSize));
        }
    }

    // These 3 fields are found in WAVEFORMATEXTENSIBLE:
    if (format.size == FormatChunkSize::WAVEFORMATEXTENSIBLE)
    {
        format.validBitsPerSample = uint16_le (in);
        format.channelMask = std::bitset<32> (uint32_le (in));
        format.subFormat = guid (in);

        if (format.subFormat != ExtensibleFormat::LinearPCM && format.subFormat != ExtensibleFormat::FloatPCM)
        {
            throw ReadError ("Extensible subformat not supported: " + format.subFormat.str());
        }
        if (format.validBitsPerSample > format.bitsPerSample)
        {
            throw ReadError ("Valid bits per sample (in WAVEFORMATEXTENSIBLE) must not be greater than bits per sample.");
        }
        if (format.subFormat == ExtensibleFormat::FloatPCM
            && format.bitsPerSample != 32 && format.bitsPerSample != 64)
        {
            throw ReadError ("For float PCM, bits per sample must be 32 or 64.");
        }
    }

    const int container_size = 8 * format.blockAlign / format.channels;
    if ((format.code == FormatCode::LinearPCM ||
         (format.code == FormatCode::Extensible && format.subFormat == ExtensibleFormat::LinearPCM))
        && container_size != 8 && container_size != 16 && container_size != 24 && container_size != 32)
    {
        throw ReadError (std::format ("Unsupported linear PCM container size: {}", container_size));
    }
    // In the future, if this code is used to read format chunks of odd-byte-length,
    // we must skip the last padding byte.
    if (format.size % 2)
    {
        fseek (in, 1, SEEK_CUR);
        if (ferror (in)) throw ReadError ("Failed to skip format chunk padding.");
    }
}

void ChunkInfo::ReadFactChunk (FILE * in)
{
    fact.present = true;
    fact.size = uint32_le (in);
    if (fact.size != 4)
    {
        throw ReadError ("fact chunk size (" + std::to_string(fact.size) + ") should be 4.");
    }
    fact.fileSize = uint32_le (in);
}

void ChunkInfo::ReadPeakChunk (FILE * in)
{
    peak.present = true;
    peak.size = uint32_le (in);
    peak.version = uint32_le (in);
    peak.timestamp = uint32_le (in);
    peak.peaks.resize ((peak.size - 8)/8);
    for (auto & p : peak.peaks)
    {
        p.value = float32_le (in);
        p.positionInFrames = uint32_le (in);
    }
}

void ChunkInfo::ReadDataChunkInfoOnly (FILE * in)
{
    // No need to read id since it has already been read and has the expected value.
    data.size = uint32_le (in);
    if (data.size % format.blockAlign != 0)
    {
        throw ReadError ("Size of data chunk is not divisible by block_align.");
    }
    data.offset = static_cast<int> (ftell (in));
    fseek (in, data.size + (data.size % 2), SEEK_CUR); // skip data (and padding byte if chunk size is odd number)
    if (ferror (in)) throw ReadError ("Failed to skip data portion of data chunk.");

    // An odd chunk size will most likely be the result of writing mono, linear PCM 8-bit or 24-bit WAV,
    // with an odd number of samples, but will also occur with a larger odd number of channels:
    // odd channels x odd sample (container) size in bytes x odd number of samples = odd number of bytes
}

std::ostream & operator<< (std::ostream & os, ChunkInfo const & info)
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

std::ostream & operator<< (std::ostream & os, RiffChunk const & riff)
{
    return os
    << "RIFF chunk" << '\n'
    << "----------" << '\n'
    << "id: " << riff.id << '\n'
    << "chunk size: " << riff.size << " ( = file size in bytes minus 8)" << '\n'
    << "format: " << riff.format;
}

/// Provides channel information based on number of channels in file and channel mask.
std::string FormatChunk::channelString() const
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

    constexpr std::array<std::string_view,18> speakerPositions
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
    size_t positions_assigned {0};
    bool first_element {true};

    constexpr std::string indent = "    ";

    // Assign speaker position strings to channels until we either
    // run out of channels or run out of speaker positions set in the channel mask.
    for (; channel < channels && positions_assigned < channelMask.count(); ++speaker_index)
    {
        if (channelMask.test (speaker_index))
        {
            ++positions_assigned;
            result += std::exchange(first_element, false) ? indent : ", ";
            result += "[" + std::to_string (channel++) + "] " + std::string(speakerPositions [speaker_index]);
        }
    }

    // Declare any channels without assigned speaker positions as "unassigned".
    for (; channel < channels; ++channel)
    {
        result += std::exchange(first_element, false) ? indent : ", ";
        result += "[" + std::to_string (channel) + "] unassigned";
    }

    // Add warning for unused speaker positions when more bits are set
    // in the channel mask than there are actual channels of audio.
    first_element = true;

    for (; positions_assigned < channelMask.count(); ++speaker_index)
    {
        if (channelMask.test (speaker_index))
        {
            ++positions_assigned;
            result += std::exchange(first_element, false) ? "\n" + indent + "No audio found for position(s) set in channel mask: " : ", ";
            result += speakerPositions [speaker_index];
        }
    }

    return result;
}

std::ostream & operator<< (std::ostream & os, FormatChunk const & format)
{
    auto description = [] (uint16_t format)
    {
        switch (format)
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
        << "extra format information size (cbSize): " << format.extraFormatInformationSize;
    }
    // WAVEFORMATEXTENSIBLE
    if (format.size == FormatChunkSize::WAVEFORMATEXTENSIBLE)
    {
        os << '\n'
        << "valid bits per sample: " << format.validBitsPerSample << '\n'
        << "channel mask: " << format.channelMask << '\n' << format.channelString() << '\n'
        << "sub format GUID: " << format.subFormat;
    }

    return os;
}

std::ostream & operator<< (std::ostream & os, DataChunk const & data)
{
    return os
    << "data chunk" << '\n'
    << "----------" << '\n'
    << "id: " << data.id << '\n'
    << "chunk size: " << data.size << '\n'
    << "[DATA]";
}

std::ostream & operator<< (std::ostream & os, FactChunk const & fact)
{
    return os
    << "fact chunk" << '\n'
    << "----------" << '\n'
    << "id: " << fact.id << '\n'
    << "chunk size: " << fact.size << '\n'
    << "file size: " << fact.fileSize;
}

std::ostream & operator<< (std::ostream & os, PeakChunk const & peak)
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

} // namespace wav
