// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_FileInfoImplementation_hpp
#define wavplusplus_FileInfoImplementation_hpp

#include "AudioSummary.hpp"

#include <cassert>

namespace wav
{

inline void FileInfo::ExtractFrom (ChunkInfo const & chunkInfo)
{
    channels = chunkInfo.format.channels;
    sampleRate = chunkInfo.format.sampleRate;
    frames = chunkInfo.data.size / chunkInfo.format.blockAlign;
    bitDepth = chunkInfo.bitDepth;
    containerSizeInBits = chunkInfo.containerSize;
    if (chunkInfo.isExtensible)
    {
        channelMask = chunkInfo.format.channelMask;
    }
    isLinearPCM = chunkInfo.isLinear;
    isFloatPCM = chunkInfo.isFloat;
}

inline std::ostream & operator<< (std::ostream & os, FileInfo const & info)
{
    const auto bytes = info.frames * info.channels * info.containerSizeInBits / 8U;
    const bool isPadded = info.containerSizeInBits > info.bitDepth;

    assert (info.isLinearPCM || info.isFloatPCM);

    os << audioSummary (info.isLinearPCM, info.bitDepth, info.containerSizeInBits,
                        info.sampleRate, info.channels, info.frames, bytes, isPadded);

    return os;
}

} // namespace wav

#endif
