
#ifndef wavplusplus_WaveFile_hpp
#define wavplusplus_WaveFile_hpp

#include "reading_bytes.hpp"

#include <string>
#include <iostream>
#include <vector>
#include <cstdint>

namespace wav
{

enum class ExportFormat
{
    LINEAR_PCM_8,
    LINEAR_PCM_16,
    LINEAR_PCM_24,
    LINEAR_PCM_32,
    FLOAT_PCM_32,
    FLOAT_PCM_64
};

namespace FormatCode // format code in "fmt " chunk
{
static constexpr uint16_t LINEAR_PCM = 0x0001;
static constexpr uint16_t FLOAT_PCM  = 0x0003;
static constexpr uint16_t EXTENSIBLE = 0xFFFE;
}

struct GUID // for WAVEFORMATEXTENSIBLE
{
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    std::array<uint8_t,8> data4;

    auto operator<=> (GUID const &) const = default; // allows for comparing two GUIDs
    std::string str() const;
};

namespace ExtensibleFormat // WAVEFORMATEXTENSIBLE sub-formats
{
static constexpr GUID LINEAR_PCM
{0x00000001, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
static constexpr GUID FLOAT_PCM
{0x00000003, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
}

struct RiffChunk
{
    std::string id {"RIFF"};
    uint32_t size; // = file size in bytes - 8
    std::string format;
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
    std::vector<int32_t> samples_int;
    std::vector<float> samples_float;
    std::vector<double> samples_double;
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

class File
{
public:
    File (std::string const & filepath, bool verbose = false);
    void PrintFileInfo();
    void PrintSamples();
    void PrintChunks();

    RiffChunk riff;
    FormatChunk format;
    DataChunk data;
    FactChunk fact;
    PeakChunk peak;

private:
    void ReadRiffChunk   (std::vector<std::byte> const &, int & position);
    void ReadFormatChunk (std::vector<std::byte> const &, int & position);
    void ReadFactChunk   (std::vector<std::byte> const &, int & position);
    void ReadPeakChunk   (std::vector<std::byte> const &, int & position);
    void ReadDataChunk   (std::vector<std::byte> const &, int & position);

    void ReadLinearPCM            (std::vector<std::byte> const &, int & position, int container_size_in_bits);
    void ReadIEEE754Float         (std::vector<std::byte> const &, int & position);
    void ReadExtensibleFormatData (std::vector<std::byte> const &, int & position);
};

std::ostream & operator<< (std::ostream & os, RiffChunk   const & riff);
std::ostream & operator<< (std::ostream & os, GUID        const & guid);
std::ostream & operator<< (std::ostream & os, FormatChunk const & format);
std::ostream & operator<< (std::ostream & os, DataChunk   const & data);
std::ostream & operator<< (std::ostream & os, FactChunk   const & fact);
std::ostream & operator<< (std::ostream & os, PeakChunk   const & peak);

} // namespace wav

#endif
