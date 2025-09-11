
#ifndef wavplusplus_ChunkInfo_hpp
#define wavplusplus_ChunkInfo_hpp

#include "ReadingBytes.hpp"

#include <string>
#include <iostream>
#include <vector>
#include <cstdint>
#include <cstdio>

namespace wav
{

struct RiffChunk
{
    std::string id {"RIFF"};
    uint32_t size {0}; // = file size in bytes - 8
    std::string format {"WAVE"};
};

struct FormatChunk
{
    // WAVEFORMAT, PCMWAVEFORMAT, WAVEFORMATEX, WAVEFORMATEXTENSIBLE:
    std::string id {"fmt "};
    uint32_t size {0};
    uint16_t code {0}; // linear PCM (0x0001), float PCM (0x0003), or extensible (0xFFFE)
    uint16_t channels {0};
    uint32_t sampleRate {0};
    uint32_t byteRate {0};
    uint16_t blockAlign {0};

    // PCMWAVEFORMAT, WAVEFORMATEX, WAVEFORMATEXTENSIBLE:
    uint16_t bitsPerSample {0};

    // WAVEFORMATEX, WAVEFORMATEXTENSIBLE:
    uint16_t extraFormatInformationSize {0}; // a.k.a. cbSize

    // WAVEFORMATEXTENSIBLE:
    uint16_t validBitsPerSample {0};
    std::bitset<32> channelMask;
    GUID subFormat;
    std::string channelString () const;
};

struct DataChunk
{
    std::string id {"data"};
    uint32_t size {0};
    int offset {0}; // first byte of first sample
};

struct FactChunk
{
    std::string id {"fact"};
    uint32_t size {0};
    uint32_t fileSize {0};
    bool present {false};
};

struct Peak
{
    float value {0}; // signed. 1.0 represents 0dB for non-floating-point encodings.
    uint32_t positionInFrames {0};
};
struct PeakChunk
{
    std::string id {"PEAK"};
    uint32_t size {0};
    uint32_t version {0};
    uint32_t timestamp {0};
    std::vector<Peak> peaks;
    bool present {false};
};

class ChunkInfo
/// All the info from RIFF, fmt, data, fact and PEAK chunks except for the samples.
/// (Essentially a header parser with the qualification that a WAV file need not be
/// constructed with the data chunk after all the others with the sample data at the
/// very end of the file and all the other information before them.)
{
public:
    ChunkInfo (std::string const & filepath);
    ChunkInfo (FILE * in);

    std::string summary() const;

    RiffChunk riff;
    FormatChunk format;
    DataChunk data;
    FactChunk fact;
    PeakChunk peak;

    std::vector<std::string> chunksEncountered;
    std::vector<std::string> chunksSkipped;

private:
    void ReadFile              (FILE * in);
    void ReadRiffChunk         (FILE * in);
    void ReadFormatChunk       (FILE * in);
    void ReadFactChunk         (FILE * in);
    void ReadPeakChunk         (FILE * in);
    void ReadDataChunkInfoOnly (FILE * in);

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
};

std::ostream & operator<< (std::ostream & os, ChunkInfo   const &);
std::ostream & operator<< (std::ostream & os, RiffChunk   const &);
std::ostream & operator<< (std::ostream & os, FormatChunk const &);
std::ostream & operator<< (std::ostream & os, DataChunk   const &);
std::ostream & operator<< (std::ostream & os, FactChunk   const &);
std::ostream & operator<< (std::ostream & os, PeakChunk   const &);

} // namespace wav

#endif
