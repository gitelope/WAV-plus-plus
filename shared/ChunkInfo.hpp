
#ifndef wavplusplus_File_hpp
#define wavplusplus_File_hpp

#include "reading_bytes.hpp"

#include <string>
#include <iostream>
#include <vector>
#include <cstdint>

namespace wav
{

struct RiffChunk
{
    std::string id {"RIFF"};
    uint32_t size; // = file size in bytes - 8
    std::string format; // WAVE
};

struct FormatChunk
{
    // WAVEFORMAT, PCMWAVEFORMAT, WAVEFORMATEX, WAVEFORMATEXTENSIBLE:
    std::string id {"fmt "};
    uint32_t size;
    uint16_t code; // linear PCM (0x0001), float PCM (0x0003), or extensible (0xFFFE)
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;

    // PCMWAVEFORMAT, WAVEFORMATEX, WAVEFORMATEXTENSIBLE:
    uint16_t bits_per_sample;

    // WAVEFORMATEX, WAVEFORMATEXTENSIBLE:
    uint16_t extra_format_information_size;

    // WAVEFORMATEXTENSIBLE:
    uint16_t valid_bits_per_sample;
    std::bitset<32> channel_mask;
    GUID sub_format;
    std::string channelString () const;
};

struct DataChunk
{
    std::string id {"data"};
    uint32_t size;
    int offset; // first byte of first sample
    //std::vector<int32_t> samples_int;
    //std::vector<float> samples_float;
    //std::vector<double> samples_double;
};

struct FactChunk
{
    std::string id {"fact"};
    uint32_t size;
    uint32_t file_size;
    bool present {false};
};

struct Peak
{
    float value; // signed. 1.0 represents 0dB for non-floating-point encodings.
    uint32_t position_in_frames;
};
struct PeakChunk
{
    std::string id {"PEAK"};
    uint32_t size;
    uint32_t version;
    uint32_t timestamp;
    std::vector<Peak> peaks;
    bool present {false};
};

/// All the info from RIFF, fmt, data, fact and PEAK chunks except for the samples.
class ChunkInfo
{
public:
    ChunkInfo (std::string const & filepath);

    std::string summary() const;

    // void PrintSamples();

    RiffChunk riff;
    FormatChunk format;
    DataChunk data;
    FactChunk fact;
    PeakChunk peak;

    std::vector<std::string> chunksEncountered;
    std::vector<std::string> chunksSkipped;

private:
    void ReadRiffChunk         (std::ifstream & in);
    void ReadFormatChunk       (std::ifstream & in);
    void ReadFactChunk         (std::ifstream & in);
    void ReadPeakChunk         (std::ifstream & in);
    void ReadDataChunkInfoOnly (std::ifstream & in);

    template <typename IntegerType>
    void RegisterChunk (std::string const & id, IntegerType position)
    {
        chunksEncountered.emplace_back (id + " " + std::to_string (position));
    }
    template <typename IntegerType>
    void RegisterSkippedChunk (std::string const & id, IntegerType position)
    {
        chunksSkipped.emplace_back (id + " " + std::to_string (position));
    }

    // void ReadLinearPCM            (std::vector<std::byte> const &, size_t & position, int container_size_in_bits);
    // void ReadIEEE754Float         (std::vector<std::byte> const &, size_t & position);
    // void ReadExtensibleFormatData (std::vector<std::byte> const &, size_t & position);
};

std::ostream & operator<< (std::ostream & os, ChunkInfo   const &);
std::ostream & operator<< (std::ostream & os, RiffChunk   const &);
std::ostream & operator<< (std::ostream & os, FormatChunk const &);
std::ostream & operator<< (std::ostream & os, DataChunk   const &);
std::ostream & operator<< (std::ostream & os, FactChunk   const &);
std::ostream & operator<< (std::ostream & os, PeakChunk   const &);

} // namespace wav

#endif
