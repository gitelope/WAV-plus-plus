
#ifndef wavplusplus_Read_hpp
#define wavplusplus_Read_hpp

#include "ChunkInfo.hpp"
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

    void ExtractFrom (ChunkInfo const &);
};

void Read (std::string const & filepath, std::vector<double> & destination, Info &, bool verbose = false);

// Compromise between speed and RAM usage with read buffer of adjustable size
void ReadDataBalanced    (std::string const & filepath, std::vector<double> & destination, Info const &, DataChunk const &, size_t requestedBufferSizeInBytes);
// Fastest read and greatest RAM usage
void ReadDataFastest     (std::string const & filepath, std::vector<double> & destination, Info const &, DataChunk const &);
// Slowest read and lowest RAM usage
void ReadDataLeastMemory (std::string const & filepath, std::vector<double> & destination, Info const &, DataChunk const &);

void ReadLinearPCM (std::vector<std::byte> const & bytes,  std::vector<double> & destination, Info const &);
void ReadLinearPCM (std::ifstream & in,                    std::vector<double> & destination, Info const &);
void ReadLinearPCM (std::vector<std::byte> const & buffer, std::vector<double> & destination, Info const &, size_t samplesToRead, size_t writePosition);

void ReadFloatPCM  (std::vector<std::byte> const & bytes,  std::vector<double> & destination, Info const &);
void ReadFloatPCM  (std::ifstream &,                       std::vector<double> & destination, Info const &);
void ReadFloatPCM  (std::vector<std::byte> const & buffer, std::vector<double> & destination, Info const &, size_t samplesToRead, size_t writePosition);

} // namespace wav

#endif
