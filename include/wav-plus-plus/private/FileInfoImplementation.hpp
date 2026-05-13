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

    // SAMPLE SIZE AND SAMPLE CONTAINER SIZE
    //
    // According to the non-formally defined WAV standard, WAVEFORMATEXTENSIBLE
    // is the only format to be used to pack linear PCM samples into larger
    // containers. bitsPerSample represents the container size while
    // validBitsPerSample represents the number of meaningful bits, a.k.a the
    // sample bit depth.
    //
    // While PCMWAVEFORMAT and WAVEFORMATEX contain bitsPerSample, they do not
    // have a validBitsPerSample. However, due to the fact that bitsPerSample
    // is redundant for linear PCM audio
    // –it can be calculated as 8 * blockAlign / channels–
    // it is possible to pack linear PCM samples into larger containers by
    // interpreting bitsPerSample as the number of valid bits per sample and
    // 8 * blockAlign / channels as the container size.
    //
    // (WAVEFORMAT, unlikely to be enountered, does not have a bitsPerSample and
    // thus the container size and the number of valid bits are one and the same
    // and calculated as 8 * blockAlign / channels.)
    //
    // The following scheme shows how WAV++ determines sample size and sample
    // container size in order to correctly parse both WAV files following the
    // standard and WAV files encoded using the non-standard packing method.
    //
    // format                 container size               sample size
    //
    // WAVEFORMAT             8 * blockAlign / channels    8 * blockAlign / channels
    // PCMWAVEFORMAT          8 * blockAlign / channels    bitsPerSample
    // WAVEFORMATEX           8 * blockAlign / channels    bitsPerSample
    // WAVEFORMATEXTENSIBLE   bitsPerSample                validBitsPerSample

    switch (chunkInfo.format.size)
    {
        case FormatChunkSize::WAVEFORMAT:
            containerSizeInBits = static_cast<uint16_t> (8 * chunkInfo.format.blockAlign / channels);
            bitDepth = containerSizeInBits;
            break;

        case FormatChunkSize::PCMWAVEFORMAT:
            containerSizeInBits = static_cast<uint16_t> (8 * chunkInfo.format.blockAlign / channels);
            bitDepth = chunkInfo.format.bitsPerSample;
            break;

        case FormatChunkSize::WAVEFORMATEX:
            containerSizeInBits = static_cast<uint16_t> (8 * chunkInfo.format.blockAlign / channels);
            bitDepth = chunkInfo.format.bitsPerSample;
            break;

        case FormatChunkSize::WAVEFORMATEXTENSIBLE:
            containerSizeInBits = chunkInfo.format.bitsPerSample;
            bitDepth = chunkInfo.format.validBitsPerSample;
            channelMask = chunkInfo.format.channelMask;
            // Note: FWIW containerSizeInBits should also be equal to 8 * chunkInfo.format.blockAlign / channels.
            break;

        default: assert (false); // should have been caught earlier
    }
    switch (chunkInfo.format.code)
    {
        case FormatCode::LinearPCM: isLinearPCM = true; break;
        case FormatCode::FloatPCM:  isFloatPCM  = true; break;
        case FormatCode::Extensible:
        {
            if      (chunkInfo.format.subFormat == ExtensibleFormat::LinearPCM) isLinearPCM = true;
            else if (chunkInfo.format.subFormat == ExtensibleFormat::FloatPCM)  isFloatPCM  = true;
            break;
        }
        default: assert (false); // should have been caught earlier
    }
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
