
#include "File.hpp"
#include <cassert>
#include <sstream>

namespace wav
{

File::File (std::string const & filepath, bool verbose)
{
    const auto bytes {ReadBytes (filepath)};

    if (verbose) std::cout << "Chunks present:";

    size_t position {0};

    const auto chunkID = fourCharacterCode (bytes, position);
    if (chunkID != "RIFF") throw std::runtime_error ("Invalid WAV format: No RIFF chunk.");
    if (verbose) std::cout << '\n' << chunkID;
    ReadRiffChunk (bytes, position);

    while (position < riff.size + 8)
    {
        const auto chunkID = fourCharacterCode (bytes, position);
        if (verbose) std::cout << '\n' << chunkID;

        if      (chunkID == "fmt ") ReadFormatChunk (bytes, position);
        else if (chunkID == "fact") ReadFactChunk   (bytes, position);
        else if (chunkID == "PEAK") ReadPeakChunk   (bytes, position);
        else if (chunkID == "data") ReadDataChunk   (bytes, position);
        else
        {
            // skip to next chunk
            const auto size = uint32_le (bytes, position);
            if (verbose) std::cout << ": index " << position - 4 << ", size " << size << " - skipped";
            position += size;
            // RIFF sub-chunks (not to be confused with "RIFF" chunks)
            // are not allowed to have an odd number of bytes.
            // If there are an odd number of meaningful bytes, that odd
            // number will be the reported size but there will be an
            // unreported byte of padding at the end to make for an even
            // number. However, all padding should be reflected in the main
            // "RIFF" chunk size.
            position += (size % 2); // When chunk size is odd, increment position by 1.
        }
    }
    if (verbose) std::cout << std::endl;

    if (position > riff.size + 8)
        throw std::runtime_error ("Ending position (" + std::to_string(position)
                                  + ") at end of file read is above expected index ("
                                  + std::to_string (riff.size + 8) + ").");
}

void File::PrintInfo()
{
    std::string channelString;
    switch (format.channels)
    {
        case 1: channelString = "mono"; break;
        case 2: channelString = "stereo"; break;
        default: channelString = std::to_string (format.channels) + " channels"; break;
    }

    const auto frames = data.size / format.block_align;
    const auto duration = frames / static_cast<double>(format.sample_rate);

    std::string format_string;

    switch (format.code)
    {
        case FormatCode::LINEAR_PCM:
        {
            format_string = std::to_string (format.bits_per_sample) + "-bit linear PCM";
            const int container_size_in_bits = 8 * format.block_align / format.channels;
            if (format.bits_per_sample < container_size_in_bits)
                format_string += std::format (" / {}-bit container", container_size_in_bits);
            break;
        }
        case FormatCode::FLOAT_PCM:
        {
            format_string = std::to_string (format.bits_per_sample) + "-bit float PCM";
            break;
        }
        case FormatCode::EXTENSIBLE:
        {
            format_string = "extensible ";
            format_string += std::to_string (format.valid_bits_per_sample) + "-bit ";

            if (format.sub_format == ExtensibleFormat::LINEAR_PCM) format_string += "linear PCM";
            else if (format.sub_format == ExtensibleFormat::FLOAT_PCM) format_string += "float PCM";
            else assert (false);

            if (format.valid_bits_per_sample < format.bits_per_sample)
                format_string += std::format (" / {}-bit container", format.bits_per_sample);
            break;
        }
        default:
            format_string = "unsupported format";
    }
    std::cout << riff.format << " "
    << format_string << " / "
    << format.sample_rate / 1000 << " kHz / "
    << channelString << " / "
    << frames << " frames" << " / "
    << duration << " seconds" << " / "
    << data.size << " bytes of audio" << std::endl;
}

void File::PrintSamples()
{
    switch (format.code)
    {
        case FormatCode::LINEAR_PCM:
        {
            for (auto const & sample : data.samples_int)
            {
                std::cout << sample << ",";
            }
            std::cout << std::endl;
            break;
        }

        case FormatCode::FLOAT_PCM:
        {
            switch (format.bits_per_sample)
            {
                case 32:
                    for (auto const & sample : data.samples_float)
                        std::cout << std::format("{}", sample) << ",";
                    std::cout << std::endl;
                    break;

                case 64:
                    for (auto const & sample : data.samples_double)
                        std::cout << std::format("{}", sample) << ",";
                    std::cout << std::endl;
                    break;
            }
            break;
        }
        case FormatCode::EXTENSIBLE:
        {
            if (format.sub_format == ExtensibleFormat::LINEAR_PCM)
            {
                for (auto const & sample : data.samples_int)
                {
                    std::cout << sample << ",";
                }
                std::cout << std::endl;
            }
            else if (format.sub_format == ExtensibleFormat::FLOAT_PCM)
            {
                switch (format.bits_per_sample)
                {
                    case 32:
                        for (auto const & sample : data.samples_float)
                            std::cout << std::format("{}", sample) << ",";
                        std::cout << std::endl;
                        break;

                    case 64:
                        for (auto const & sample : data.samples_double)
                            std::cout << std::format("{}", sample) << ",";
                        std::cout << std::endl;
                        break;
                }
            }
            else assert (false);
            break;
        }
    }
}

void File::PrintChunks()
{
    std::cout << '\n' << riff << "\n\n" << format << "\n\n";
    if (fact.present)
        std::cout << fact << "\n\n";
    if (peak.present)
        std::cout << peak << "\n\n";
    std::cout << data << std::endl;
}

void File::ReadRiffChunk (std::vector<std::byte> const & bytes, size_t & position)
{
    // Technically, the whole file is a RIFF chunk. This is just the first part of it.
    assert (position == 4);
    // no need to read id since it has already been read and has the expected value
    riff.size = uint32_le (bytes, position);
    riff.format = fourCharacterCode (bytes, position);
    if (riff.format != "WAVE") throw std::runtime_error ("The format in the RIFF chunk for WAV files should be \"WAVE\".");
    assert (position == 12);
}

void File::ReadFormatChunk (std::vector<std::byte> const & bytes, size_t & position)
{
    // These fields are found in formats WAVEFORMAT, PCMWAVEFORMAT, WAVEFORMATEX, and WAVEFORMATEXTENSIBLE:
    //
    // no need to read ID since it has already been read and has the expected value
    //
    format.size = uint32_le (bytes, position);
    // format chunk size
    // WAVEFORMAT:                                                       14
    // PCMWAVEFORMAT:                                             14+2 = 16
    // WAVEFORMATEX:                                            14+2+2 = 18
    // WAVEFORMATEXTENSIBLE (signed int PCM and IEEE float): 14+2+2+22 = 40
    if (format.size != 14 && format.size != 16 && format.size != 18 && format.size != 40)
    {
        throw std::runtime_error ("fmt chunk size (" + std::to_string (format.size) + ") is unsupported.");
    }
    format.code = uint16_le (bytes, position);
    if (format.code != FormatCode::LINEAR_PCM && format.code != FormatCode::FLOAT_PCM && format.code != FormatCode::EXTENSIBLE)
    {
        throw std::runtime_error (std::format ("Format not supported: 0x{:04X}", format.code));
    }
    format.channels = uint16_le (bytes, position);
    format.sample_rate = uint32_le (bytes, position);
    format.byte_rate = uint32_le (bytes, position);
    format.block_align = uint16_le (bytes, position);

    // This field is found in PCMWAVEFORMAT, WAVEFORMATEX, and WAVEFORMATEXTENSIBLE:
    if (format.size == 16 || format.size == 18 || format.size == 40)
    {
        format.bits_per_sample = uint16_le (bytes, position);

        if (format.code == FormatCode::EXTENSIBLE)
        {
            // Since WAVEFORMATEXTENSIBLE has bits_per_sample AND valid_bits_per_sample,
            // it makes good sense even for a permissive parser to enforce the following.
            //     1. bits_per_sample must be a multiple of 8
            //     2. bits_per_sample = 8 * block_align / channels
            if (format.bits_per_sample % 8 != 0)
                throw std::runtime_error ("For WAVEFORMATEXTENSIBLE, bits per sample must be a multiple of 8.");
            if (format.bits_per_sample != 8 * format.block_align / format.channels)
                throw std::runtime_error ("For WAVEFORMATEXTENSIBLE, bits per sample must equal 8 * block_align / channels.");
        }
        else if (format.code == FormatCode::FLOAT_PCM)
        {
            // Assuming for reasonably well-formed float PCM WAVs (of format PCMFORMAT and WAVEFORMATEX):
            //     1. bits_per_sample will equal either 32 or 64
            //     2. bits_per_sample = 8 * block_align / channels
            if (format.bits_per_sample != 32 && format.bits_per_sample != 64)
                throw std::runtime_error ("For float PCM, bits per sample must be 32 or 64.");
            if (format.bits_per_sample != 8 * format.block_align / format.channels)
                throw std::runtime_error ("For float PCM, bits per sample must equal 8 * block_align / channels.");
        }

        if ((format.code == FormatCode::EXTENSIBLE || format.code == FormatCode::FLOAT_PCM) && format.bits_per_sample != 8 * format.block_align / format.channels)
        {
            throw std::runtime_error ("bits_per_sample should equal 8 * block_align / channels for this format.");
            // This doesn't apply to LINEAR_PCM in order to support non-standard usage in which meaningful bit depth is lower
            // than container size and yet the WAV is not of the format WAVEFORMATEXTENSIBLE.
            // In this scenario, bits_per_sample is made to equal the number of significant bits and is less than
            // 8 * format.block_align / format.channels (the container size). For uncompressed linear and float PCM,
            // (stored in PCMWAVEFORMAT, WAVEFORMATEX, and WAVEFORMATEXTENSIBLE) these should be equal in standard usage.
        }
    }

    // This field is found in WAVEFORMATEX and WAVEFORMATEXTENSIBLE:
    // WAVEFORMATEX should have a format chunk size of 18 and a cbSize value of 0 for signed int PCM and float.
    // WAVEFORMATEXTENSIBLE should have a format chunk size of 40 (18 + 22) and a cbSize of 22.
    if (format.size == 18 || format.size == 40)
    {
        format.extra_format_information_size = uint16_le (bytes, position);
        if (format.extra_format_information_size != format.size - 18)
            throw std::runtime_error("Invalid cbSize: " + std::to_string (format.extra_format_information_size));
    }

    // These 3 fields are found in WAVEFORMATEXTENSIBLE:
    if (format.size == 40)
    {
        format.valid_bits_per_sample = uint16_le (bytes, position);
        format.channel_mask = std::bitset<32> (uint32_le (bytes, position));
        format.sub_format = guid (bytes, position);

        if (format.sub_format != ExtensibleFormat::LINEAR_PCM && format.sub_format != ExtensibleFormat::FLOAT_PCM)
            throw std::runtime_error ("Extensible subformat not supported: " + format.sub_format.str());
        if (format.valid_bits_per_sample > format.bits_per_sample)
            throw std::runtime_error ("Valid bits per sample (in WAVEFORMATEXTENSIBLE) must not be greater than bits per sample.");
        if (format.sub_format == ExtensibleFormat::FLOAT_PCM && format.bits_per_sample != 32 && format.bits_per_sample != 64)
            throw std::runtime_error ("For float PCM, bits per sample must be 32 or 64.");
    }

    const int container_size = 8 * format.block_align / format.channels;
    if ((format.code == FormatCode::LINEAR_PCM || (format.code == FormatCode::EXTENSIBLE && format.sub_format == ExtensibleFormat::LINEAR_PCM))
        && container_size != 8 && container_size != 16 && container_size != 24 && container_size != 32)
    {
        throw std::runtime_error (std::format ("Unsupported linear PCM container size: {}", container_size));
    }
    // In the future, if this code is used to read format chunks of odd-byte-length, we must skip the last padding byte.
    position += (format.size % 2); // When chunk size is odd, increment position by 1.
}

void File::ReadFactChunk (std::vector<std::byte> const & bytes, size_t & position)
{
    fact.present = true;
    fact.size = uint32_le (bytes, position);
    if (fact.size != 4) throw std::runtime_error ("fact chunk size (" + std::to_string(fact.size) + ") should be 4.");
    fact.file_size = uint32_le (bytes, position);
}

void File::ReadPeakChunk (std::vector<std::byte> const & bytes, size_t & position)
{
    peak.present = true;
    peak.size = uint32_le (bytes, position);
    peak.version = uint32_le (bytes, position);
    peak.timestamp = uint32_le (bytes, position);
    peak.peaks.resize ((peak.size - 8)/8);
    for (auto & p : peak.peaks)
    {
        p.value = float32_le (bytes, position);
        p.position_in_frames = uint32_le (bytes, position);
    }
}

void File::ReadDataChunk (std::vector<std::byte> const & bytes, size_t & position)
{
    // No need to read id since it has already been read and has the expected value.
    data.size = uint32_le (bytes, position);
    if (data.size % format.block_align != 0)
    {
        throw std::runtime_error ("Size of data chunk is not divisible by block_align.");
    }

    switch (format.code)
    {
        case FormatCode::LINEAR_PCM:
        {
            // In non-standard usage, PCMWAVEFORMAT and WAVEFORMATEX are used to store
            // samples of bit-depth lower than that of the container with:
            //
            // container size    =   8 * format.block_align / format.channels
            // significant bits  =   bits_per_sample
            //
            // (In "standard" usage of PCMWAVEFORMAT and WAVEFORMATEX, bits_per_sample
            // should instead equal 8 * format.block_align / format.channels.)
            //
            // The "standard" way to store samples in larger containers is to use the
            // WAVEFORMATEXTENSIBLE format with bits_per_sample being the container
            // size and valid_bits_per_sample being the number of significant bits.
            // This is in contrast with non-standard usage of bits_per_samples as the
            // number of valid bits.
            //
            // In either case, the significant bits are assumed to have been right-justified
            // towards the LSB with sign extension padding towards the MSB so that the sample
            // bytes can be interpreted as a signed int of the same size as the container but
            // with the original value preserved.
            //
            // The container size is needed when parsing.
            //
            // The number of significant bits is needed when dequantizing from signed int to
            // normalized [-1.0, 1.0] floating point.
            //
            const int container_size_in_bits = 8 * format.block_align / format.channels;
            ReadLinearPCM (bytes, position, container_size_in_bits);
            break;
        }
        case FormatCode::FLOAT_PCM:  ReadIEEE754Float         (bytes, position); break;
        case FormatCode::EXTENSIBLE: ReadExtensibleFormatData (bytes, position); break;
    }

    // If the chunk size is odd, there will be a padding byte of 0x00,
    // e.g. 24-bit samples require 3 bytes and therefore a mono file with seven samples
    // implies a chunk size of 1 x 7 x 3 = 21 bytes,
    // so an extra byte will be appended to the end of the data.
    // After reading the 21 bytes, we must skip the next "unreported" 22nd padding byte.
    position += (data.size % 2); // When chunk size is odd, increment position by 1.
}

void File::ReadExtensibleFormatData (std::vector<std::byte> const & bytes, size_t & position)
{
    if (format.sub_format == ExtensibleFormat::LINEAR_PCM)
        ReadLinearPCM (bytes, position, format.bits_per_sample);

    else if (format.sub_format == ExtensibleFormat::FLOAT_PCM)
        ReadIEEE754Float (bytes, position);
}

void File::ReadLinearPCM (std::vector<std::byte> const & bytes, size_t & position, int container_size_in_bits)
{
    data.samples_int.resize (data.size * format.channels / format.block_align);

    switch (container_size_in_bits)
    {
        case 8:
            for (auto & sample : data.samples_int)
                sample = uint8 (bytes, position);
            break;

        case 16:
            for (auto & sample : data.samples_int)
                sample = int16_le (bytes, position);
            break;

        case 24:
            for (auto & sample : data.samples_int)
                sample = int24_le (bytes, position);
            break;

        case 32:
            for (auto & sample : data.samples_int)
                sample = int32_le (bytes, position);
            break;
    }
}

void File::ReadIEEE754Float (std::vector<std::byte> const & bytes, size_t & position)
{
    const int N = data.size * format.channels / format.block_align;

    switch (format.bits_per_sample)
    {
        case 32:
            data.samples_float.resize (N);
            for (auto & sample : data.samples_float)
                sample = float32_le (bytes, position);
            break;

        case 64:
            data.samples_double.resize (N);
            for (auto & sample : data.samples_double)
                sample = float64_le (bytes, position);
            break;
    }
}

std::ostream & operator<< (std::ostream & os, RiffChunk const & riff)
{
    return os
    << "RIFF CHUNK" << '\n'
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

    int channel = 0;
    int speaker_index = 0;
    int positions_assigned = 0;
    bool first_element = true;

    constexpr std::string indent = "    ";

    // Assign speaker position strings to channels until we either
    // run out of channels or run out of speaker positions set in the channel mask.
    for (; channel < channels && positions_assigned < channel_mask.count(); ++speaker_index)
    {
        if (channel_mask.test (speaker_index))
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

    for (; positions_assigned < channel_mask.count(); ++speaker_index)
    {
        if (channel_mask.test (speaker_index))
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
            case FormatCode::LINEAR_PCM: return "linear PCM";
            case FormatCode::FLOAT_PCM: return "IEEE float";
            case FormatCode::EXTENSIBLE: return "extensible";
            default: return "unknown";
        }
    };

    os
    << "FORMAT CHUNK" << '\n'
    << "------------" << '\n'
    // WAVEFORMAT, PCMWAVEFORMAT, WAVEFORMATEX, WAVEFORMATEXTENSIBLE
    << "format id: "  << format.id << '\n'
    << "format chunk size: " << format.size << '\n'
    << "format: " << std::format ("0x{:04X} ({})", format.code, description (format.code)) << '\n'
    << "channels: " << format.channels << '\n'
    << "sample rate: " << format.sample_rate << '\n'
    << "byte rate: " << format.byte_rate << '\n'
    << "block align: " << format.block_align;
    // PCMWAVEFORMAT, WAVEFORMATEX, WAVEFORMATEXTENSIBLE
    if (format.size == 16 || format.size == 18 || format.size == 40)
    {
        os << '\n'
        << "bits per sample: " << format.bits_per_sample;
    }
    // WAVEFORMATEX, WAVEFORMATEXTENSIBLE
    if (format.size == 18 || format.size == 40)
    {
        os << '\n'
        << "extra format information size (cbSize): " << format.extra_format_information_size;
    }
    // WAVEFORMATEXTENSIBLE
    if (format.size == 40)
    {
        os << '\n'
        << "valid bits per sample: " << format.valid_bits_per_sample << '\n'
        << "channel mask: " << format.channel_mask << '\n' << format.channelString() << '\n'
        << "sub format GUID: " << format.sub_format;
    }

    return os;
}

std::ostream & operator<< (std::ostream & os, DataChunk const & data)
{
    return os
    << "DATA CHUNK" << '\n'
    << "----------" << '\n'
    << "id: " << data.id << '\n'
    << "chunk size: " << data.size << '\n'
    << "[DATA]";
}

std::ostream & operator<< (std::ostream & os, FactChunk const & fact)
{
    return os
    << "FACT CHUNK" << '\n'
    << "----------" << '\n'
    << "id: " << fact.id << '\n'
    << "chunk size: " << fact.size << '\n'
    << "file size: " << fact.file_size;
}

std::ostream & operator<< (std::ostream & os, PeakChunk const & peak)
{
    os
    << "PEAK CHUNK" << '\n'
    << "----------" << '\n'
    << "id: " << peak.id << '\n'
    << "chunk size: " << peak.size << '\n'
    << "version: " << peak.version  << '\n'
    << "timestamp: " << peak.timestamp << '\n'
    << "peaks: ";

    int channel = 0;
    for (auto const & p : peak.peaks)
    {
        os << "\nchannel " + std::to_string (channel++) + ", frame " + std::to_string (p.position_in_frames) + ": " + std::to_string (p.value);
    }

    return os;
}

} // namespace wav
