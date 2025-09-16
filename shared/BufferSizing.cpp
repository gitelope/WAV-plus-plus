
#include "BufferSizing.hpp"
#include <cassert>
#include <algorithm>

namespace wav
{

size_t bufferSizeInBytes (size_t requestedBufferSizeInBytes, size_t numSamples, int bitsPerSample)
{
    assert (bitsPerSample % 8 == 0);
    const auto sampleSizeInBytes {static_cast<size_t> (bitsPerSample / 8)};
    const auto totalBytes {numSamples * sampleSizeInBytes};

    // Determine buffer size from requested size, sample size, and number of samples:
    // 1. Buffer must be able to hold an entire sample's worth of bytes.
    size_t bytesPerBuffer = std::max (requestedBufferSizeInBytes, sampleSizeInBytes);
    // 2. Buffer size must correspond to a whole number of samples.
    bytesPerBuffer = (bytesPerBuffer / sampleSizeInBytes) * sampleSizeInBytes;
    // 3. Buffer size must not exceed the total number of bytes to write.
    bytesPerBuffer = std::min (bytesPerBuffer, totalBytes);

    return bytesPerBuffer;
}

size_t ioBufferSizeInBytes (size_t numSamples, int bitsPerSample,
                            size_t bufferSizeInBytes, size_t ioBufferSizeMultiplier)
{
    // Calculate internal I/O buffer as a multiple of the actual (not requested) write buffer size, making sure to clamp at file size.
    const auto totalBytesToWrite {numSamples * static_cast<size_t>(bitsPerSample / 8)};
    return std::min (ioBufferSizeMultiplier * bufferSizeInBytes, totalBytesToWrite);
}

} // namespace wav
