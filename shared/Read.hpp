
#ifndef wavplusplus_Read_hpp
#define wavplusplus_Read_hpp

#include "ChunkInfo.hpp"
#include <string>
#include <vector>

namespace wav
{

// 32kB default (requested) buffer size
inline constexpr size_t defaultRequestedReadBufferSize = 32768;

template <typename T>
concept FloatingPoint = std::same_as <T,float> || std::same_as <T,double>;

// Returns pair containing interleaved audio vector and Info.
// Calls in-place Read() overload.
// May throw wav::ReadError.
template <FloatingPoint T>
std::pair <std::vector<T>, struct Info>
ReadInterleaved (std::string const & filepath,
                 size_t requestedBufferSize = defaultRequestedReadBufferSize);

// Returns pair containing audio separated into channels and Info.
// Calls in-place Read() overload.
// May throw wav::ReadError.
template <FloatingPoint T>
std::pair <std::vector<std::vector<T>>, struct Info>
Read (std::string const & filepath,
      size_t requestedBufferSize = defaultRequestedReadBufferSize);


// In-place read of interleaved audio and Info.
// May throw wav::ReadError.
template <FloatingPoint T>
void ReadInterleaved (std::string const & filepath,
                      std::vector<T> & destination,
                      struct Info & info,
                      size_t requestedBufferSize = defaultRequestedReadBufferSize);

// In-place read to separate channels and Info.
// May throw wav::ReadError.
template <FloatingPoint T>
void Read (std::string const & filepath,
           std::vector<std::vector<T>> & destination,
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

} // namespace wav

#endif
