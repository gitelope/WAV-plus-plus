
#ifndef wavplusplus_Read_hpp
#define wavplusplus_Read_hpp

#include "ChunkInfo.hpp"
#include <string>
#include <vector>

namespace wav
{

// May throw wav::ReadError.
void Read (std::string const & filepath,
           std::vector<double> & destination,
           struct Info & info,
           size_t requestedBufferSize = 32768); // 32kB default buffer size

struct Info
{
    // playback
    int channels {0};
    int sampleRate {0};
    int frames {0};
    // total samples = channels * frames
    // length in seconds = frames / sampleRate

    // sample encoding
    int bitDepth {0}; // bits per sample
    int containerSizeInBits {0}; // sample containerSize >= sample bitDepth
    bool isLinearPCM {false};
    bool isFloatPCM {false};

    void ExtractFrom (ChunkInfo const &);
};
std::ostream & operator<< (std::ostream & os, Info const &);


////////////////////////////////////////////////////////
// HELPER FUNCTIONS
// (may throw wav::ReadError)

// Reads in a buffer of bytes (of adjustable size), decodes, and repeats until done.
void ReadData (std::string const & filepath,
               std::vector<double> & destination,
               Info const & info,
               DataChunk const & dataChunk,
               size_t requestedBufferSizeInBytes);

void ReadLinearPCM (std::vector<std::byte> const & buffer,
                    std::vector<double> & destination,
                    Info const & info,
                    size_t samplesToRead,
                    size_t writePosition);

void ReadFloatPCM  (std::vector<std::byte> const & buffer,
                    std::vector<double> & destination,
                    Info const & info,
                    size_t samplesToRead,
                    size_t writePosition);

} // namespace wav

#endif
