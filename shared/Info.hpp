

#ifndef wavplusplus_Info_hpp
#define wavplusplus_Info_hpp

#include "ChunkInfo.hpp"

namespace wav
{

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

