

#ifndef wavplusplus_Info_hpp
#define wavplusplus_Info_hpp

#include "ChunkInfo.hpp"

namespace wav
{

struct Info
{
    // playback
    uint16_t channels {0};
    uint32_t sampleRate {0};
    uint32_t frames {0};
    // total samples = channels * frames
    // length in seconds = frames / sampleRate

    // sample encoding (in the source WAV file)
    uint16_t bitDepth {0}; // bits per sample
    uint16_t containerSizeInBits {0}; // sample containerSize >= sample bitDepth
    bool isLinearPCM {false};
    bool isFloatPCM {false};

    void ExtractFrom (ChunkInfo const &);
};

std::ostream & operator<< (std::ostream & os, Info const &);

} // namespace wav

#endif

