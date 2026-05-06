// WAV++ (c) 2025-2026 David Adams
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

/*
inline size_t bufferSizeInBytes (size_t requestedBufferSizeInBytes, size_t numSamples, uint16_t bitsPerSample)
{
    assert (bitsPerSample % 8 == 0);
    const size_t bytesPerSample {bitsPerSample / 8U};
    const auto totalBytes {numSamples * bytesPerSample};

    // Determine buffer size from requested size, sample size, and number of samples:
    // 1. Buffer must be able to hold an entire sample's worth of bytes.
    size_t bytesPerBuffer = std::max (requestedBufferSizeInBytes, bytesPerSample);
    // 2. Buffer size must correspond to a whole number of samples.
    bytesPerBuffer = (bytesPerBuffer / bytesPerSample) * bytesPerSample;
    // 3. Buffer size must not exceed the total number of bytes to write.
    bytesPerBuffer = std::min (bytesPerBuffer, totalBytes);

    return bytesPerBuffer;
}

inline size_t bufferSizeInBytes (size_t requestedBufferSizeInBytes, size_t numSamples, uint16_t bitsPerSample, uint32_t numChannels)
{
    assert (bitsPerSample % 8 == 0);
    const size_t bytesPerSample {bitsPerSample / 8U};
    const size_t bytesPerFrame {numChannels * bytesPerSample};
    const auto totalBytes {numSamples * bytesPerSample};

    // Determine buffer size from requested size, sample size, number of samples, and number of channels:
    // 1. Buffer must be able to hold an entire frame's worth of bytes.
    size_t bytesPerBuffer = std::max (requestedBufferSizeInBytes, bytesPerFrame);
    // 2. Buffer size must correspond to a whole number of frames.
    bytesPerBuffer = (bytesPerBuffer / bytesPerFrame) * bytesPerFrame;
    // 3. Buffer size must not exceed the total number of bytes to write.
    bytesPerBuffer = std::min (bytesPerBuffer, totalBytes);
    return bytesPerBuffer;
}

inline size_t ioBufferSizeInBytes (size_t numSamples, uint16_t bitsPerSample,
                                   size_t bufferSizeInBytes, size_t ioBufferSizeMultiplier)
{
    // Calculate internal I/O buffer as a multiple of the actual (not requested) write buffer size, making sure to clamp at file size.
    const size_t totalBytesToWrite {numSamples * (bitsPerSample / 8U)};
    return std::min (ioBufferSizeMultiplier * bufferSizeInBytes, totalBytesToWrite);
}

template <typename ExceptionType>
void SetInternalIOBufferSize (std::FILE * out, size_t ioBufferSizeInBytes)
{
    if (std::setvbuf (out, nullptr, _IOFBF, ioBufferSizeInBytes) != 0)
    {
        throw ExceptionType ("Failed to set file buffer.");
    }
}
*/

} // namespace wav

#endif
