
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
void ReadDataFastest (std::string const & filepath, std::vector<double> & destination, Info const &, DataChunk const &);
void ReadLinearPCM (std::vector<std::byte> const & bytes, std::vector<double> & destination, Info const &, DataChunk const &);
void ReadFloatPCM  (std::vector<std::byte> const & bytes, std::vector<double> & destination, Info const &);

} // namespace wav

#endif
