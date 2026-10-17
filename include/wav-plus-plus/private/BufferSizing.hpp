// WAV++
// Copyright (c) 2025 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_BufferSizing_hpp
#define wavplusplus_BufferSizing_hpp

#include <algorithm>
#include <cstdint>

namespace wav
{

inline uint32_t bufferSize (uint32_t requestedSizeKB,
                            uint32_t dataChunkSizeInBytes,
                            uint16_t containerBits,
                            uint16_t channels)
{
    const auto bytesPerSample {containerBits / 8U};
    const auto bytesPerFrame  {bytesPerSample * channels};
    // Upcast to 64-bit to avoid wrapping (before clamping to the 32-bit data chunk size).
    const auto requestedBytes {static_cast<uint64_t>(requestedSizeKB) * 1024U};

    // Buffer constraints:
    //
    // 1. At least one frame's worth of bytes:
    auto bytesPerBuffer {std::max (requestedBytes, static_cast<uint64_t>(bytesPerFrame))};
    // 2. Length is a whole number of frames:
    bytesPerBuffer = (bytesPerBuffer / bytesPerFrame) * bytesPerFrame;
    // 3. Buffer is not longer than the number of bytes in the data chunk:
    bytesPerBuffer = std::min (bytesPerBuffer, static_cast<uint64_t>(dataChunkSizeInBytes));

    // Return container sample count
    return static_cast<uint32_t>(bytesPerBuffer / bytesPerSample);
}

} // namespace wav

#endif
