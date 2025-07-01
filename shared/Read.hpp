
#ifndef wavplusplus_Read_hpp
#define wavplusplus_Read_hpp

#include <string>
#include <vector>

namespace wav
{

struct Info
{
    int channels {0};
    int sampleRate {0};
    // total samples = channels * frames
    // length in seconds = frames / sampleRate
    int frames {0};
    int bitDepth {0}; // bits per sample
    int containerSizeInBits {0}; // sample containerSize >= sample bitDepth
    bool isLinearPCM {false};
    bool isFloatPCM {false};
    std::vector<double> peaks;
};

struct ReadInfo // holds info useful to helpers of Read() but not to its clients
{
    int riffChunkSize {0};
    int blockAlign {0};
    // int framesAccordingToFactChunk {0};
};

void Read (std::string const & filepath, std::vector<double> & destination, Info &, bool verbose = false);

void ReadRiffChunk   (std::ifstream &, ReadInfo &, bool verbose); // just reads the beginning, actually
void ReadSubchunks   (std::ifstream &, std::vector<double> & destination, Info &, ReadInfo &, bool verbose);
void ReadFormatChunk (std::ifstream &, Info &, ReadInfo &);
void ReadFactChunk   (std::ifstream &, Info &);
void ReadDataChunk   (std::ifstream &, std::vector<double> & destination, Info & info, ReadInfo & readInfo);

void ReadLinearPCM (std::ifstream &, std::vector<double> & destination, Info const &);
void ReadFloatPCM  (std::ifstream &, std::vector<double> & destination, Info const &);

} // namespace wav

#endif
