
#ifndef wavplusplus_Read_hpp
#define wavplusplus_Read_hpp

#include "ChunkInfo.hpp"
#include <string>
#include <vector>

namespace wav
{

// 32kB default (requested) buffer size
inline constexpr size_t defaultRequestedReadBufferSize = 32768;

// Returns pair containing interleaved audio vector and Info.
// Calls in-place Read() overload.
// May throw wav::ReadError.
std::pair <std::vector<double>, struct Info>
ReadInterleaved (std::string const & filepath,
                 size_t requestedBufferSize = defaultRequestedReadBufferSize);

// Returns pair containing audio separated into channels and Info.
// Calls in-place Read() overload.
// May throw wav::ReadError.
std::pair <std::vector<std::vector<double>>, struct Info>
Read (std::string const & filepath,
      size_t requestedBufferSize = defaultRequestedReadBufferSize);

// In-place read of interleaved audio and Info.
// May throw wav::ReadError.
void Read (std::string const & filepath,
           std::vector<double> & destination,
           struct Info & info,
           size_t requestedBufferSize = defaultRequestedReadBufferSize);

// In-place read to separate channels and Info.
// May throw wav::ReadError.
void Read (std::string const & filepath,
           std::vector<std::vector<double>> & destination,
           struct Info & info,
           size_t requestedBufferSize = defaultRequestedReadBufferSize);

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

void ReadData (std::string const & filepath,
               std::vector <std::vector<double>> & destination,
               Info const & info,
               DataChunk const & data,
               size_t requestedBufferSizeInBytes);

void ReadLinearPCM (std::vector<std::byte> const & buffer,
                    std::vector<double> & destination,
                    Info const & info,
                    size_t samplesToRead,
                    size_t writePosition);

void ReadLinearPCM (std::vector<std::byte> const & buffer,
                    std::vector<std::vector<double>> & destination,
                    Info const & info,
                    size_t frameToRead,
                    size_t writePosition);

void ReadFloatPCM  (std::vector<std::byte> const & buffer,
                    std::vector<double> & destination,
                    Info const & info,
                    size_t samplesToRead,
                    size_t writePosition);

void ReadFloatPCM  (std::vector<std::byte> const & buffer,
                    std::vector<std::vector<double>> & destination,
                    Info const & info,
                    size_t framesToRead,
                    size_t writePosition);

} // namespace wav

#endif
