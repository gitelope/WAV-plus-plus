
#ifndef wavplusplus_Load_hpp
#define wavplusplus_Load_hpp

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

void Read (std::string const & filepath, std::vector<double> & destination, Info &);

void ReadRiffChunk   (std::ifstream &, int & position, ReadInfo &); // just reads the beginning, actually
void ReadSubchunks   (std::ifstream &, int & position, std::vector<double> & destination, Info &, ReadInfo &);
void ReadFormatChunk (std::ifstream &, int & position, Info &, ReadInfo &);
void ReadFactChunk   (std::ifstream &, int & position, Info &);
void ReadDataChunk   (std::ifstream &, int & position, std::vector<double> & destination, Info & info, ReadInfo & readInfo);

void ReadLinearPCM (std::ifstream &, int & position, std::vector<double> & destination, Info const &);
void ReadFloatPCM  (std::ifstream &, int & position, std::vector<double> & destination, Info const &);

} // namespace wav

#endif
