
#ifndef wavplusplus_BufferSizing_hpp
#define wavplusplus_BufferSizing_hpp

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>

namespace wav
{

inline size_t bufferSizeInBytes (size_t requestedBufferSizeInBytes, size_t numSamples, uint16_t bitsPerSample)
{
    assert (bitsPerSample % 8 == 0);
    const size_t sampleSizeInBytes {bitsPerSample / 8U};
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

size_t ioBufferSizeInBytes (size_t numSamples, uint16_t bitsPerSample,
                            size_t bufferSizeInBytes, size_t ioBufferSizeMultiplier)
{
    // Calculate internal I/O buffer as a multiple of the actual (not requested) write buffer size, making sure to clamp at file size.
    const size_t totalBytesToWrite {numSamples * (bitsPerSample / 8U)};
    return std::min (ioBufferSizeMultiplier * bufferSizeInBytes, totalBytesToWrite);
}

template <typename ExceptionType>
void SetInternalIOBufferSize (FILE * out, size_t ioBufferSizeInBytes)
{
    if (setvbuf (out, nullptr, _IOFBF, ioBufferSizeInBytes) != 0)
    {
        throw ExceptionType ("Failed to set file buffer.");
    }
}

} // namespace wav

#endif
