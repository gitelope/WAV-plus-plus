
#ifndef wavplusplus_FileInfo_hpp
#define wavplusplus_FileInfo_hpp

#include "ChunkInfo.hpp"

#include <cstdint>
#include <ostream>

namespace wav
{

struct FileInfo
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

inline std::ostream & operator<< (std::ostream & os, FileInfo const & info);

} // namespace wav

#include "private/FileInfoImplementation.hpp"

#endif

