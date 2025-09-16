
#ifndef wavplusplus_BufferSizing_hpp
#define wavplusplus_BufferSizing_hpp

#include <cstdio>
#include <cstdint>

namespace wav
{

size_t bufferSizeInBytes (size_t requestedBufferSizeInBytes, size_t numSamples, int bitsPerSample);

size_t ioBufferSizeInBytes (size_t numSamples, int bitsPerSample, size_t bufferSizeInBytes, size_t ioBufferSizeMultiplier);

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
