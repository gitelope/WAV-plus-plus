
#ifndef wavplusplus_Writer_hpp
#define wavplusplus_Writer_hpp

#include "Formats.hpp"
#include "Exceptions.hpp"
#include "private/QuantizationConstants.hpp"
#include "private/BufferSizing.hpp"
#include "private/int24.hpp"

#include <cassert>
#include <cmath>
#include <vector>
#include <cstdio>
#include <bit>
#include <string>
#include <algorithm>
#include <memory>

namespace wav
{

class Writer
{

public:

    Writer (std::string const & filepath,
            ExportFormat exportFormat,
            uint32_t sampleRate,
            uint16_t channels,
            size_t bufferSizeInKB = 32);

    ~Writer(); // Closes file if client does not.

    // Write interleaved samples from user-provided buffer.
    // Returns the number of samples written.
    // May throw wav::WriteError.
    template <FloatOrDouble T>
    size_t write (T const * source, size_t samplesToWrite);

    // write() convenience overloads
    //
    // fixed-size C-style array (write all samples)
    // May throw wav::WriteError.
    template <FloatOrDouble T, size_t N>
    size_t write (const T (&source)[N]);
    //
    // Main overload of write() is called for all C-style arrays,
    // so both fixed-sized and dynamically-sized,
    // when samples to write is provided.
    //
    // std::vector (write all samples)
    // May throw wav::WriteError.
    template <FloatOrDouble T>
    size_t write (std::vector<T> const & source);
    //
    // std::vector (write number of samples specified)
    // May throw wav::WriteError.
    template <FloatOrDouble T>
    size_t write (std::vector<T> const & source, size_t samplesToWrite);
    //
    // std::array (write all samples)
    // May throw wav::WriteError.
    template <FloatOrDouble T, size_t N>
    size_t write (std::array<T,N> const & source);
    //
    // std::array (write number of samples specified)
    // May throw wav::WriteError.
    template <FloatOrDouble T, size_t N>
    size_t write (std::array<T,N> const & source, size_t samplesToWrite);

    // A convenience method for specifying number of frames to write
    // instead of number of samples. Returns the number of frames written.
    // May throw wav::WriteError.
    template <FloatOrDouble T>
    size_t writeFrames (T const * source, size_t framesToWrite);

    bool isOpen() const { return file != nullptr; }

    // Manual finalization allows client to handle exceptions.
    // Destructor will call if client doesn't, swallowing any exceptions.
    void Close();

private:

    std::unique_ptr<FILE, decltype(&fclose)> file;

    uint16_t channels;
    uint16_t bitsPerSample;
    uint16_t formatCode;

    size_t totalSamplesWritten {0};
    // Largest allowable data chunk given header size, encoding, channels,
    // limit of 32-bit unsigned data chunk size, that ensures a whole
    // number of frames are written.
    uint32_t maxDataChunkSize {0};

    // Header field positions set by WriteHeader()
    // to be used later to replace placeholders.
    long riffChunkSizePosition {-1};
    long dataChunkSizePosition {-1};

    // Internal buffers for different output encodings.
    std::vector<uint8_t> bufferLPCMU8;
    std::vector<int16_t> bufferLPCM16;
    std::vector<int24>   bufferLPCM24;
    std::vector<int32_t> bufferLPCM32;
    std::vector<float>   bufferFloatPCM32;
    std::vector<double>  bufferFloatPCM64;

    void WriteHeader (uint32_t sampleRate);
    void SetMaximumDataChunkSize (uint16_t blockAlign);

    void InitializeBuffer (size_t sizeInBytes);

    // float PCM
    template <FloatOrDouble T> void WriteFloatPCM_NoConversion (T const * source, size_t samplesToWrite);
    void WriteFloatToDouble (float const * source, size_t samplesToWrite);
    void WriteDoubleToFloat (double const * source, size_t samplesToWrite);

    // linear PCM
    template <FloatOrDouble T> void WriteLPCMU8 (T const * source, size_t samplesToWrite);
    template <FloatOrDouble T> void WriteLPCM16 (T const * source, size_t samplesToWrite);
    template <FloatOrDouble T> void WriteLPCM24 (T const * source, size_t samplesToWrite);
    template <FloatOrDouble T> void WriteLPCM32 (T const * source, size_t samplesToWrite);
    
    // Updates both data and RIFF chunk sizes.
    // Assumes any necessary padding of data chunk's data has been added.
    void UpdateHeader (uint32_t dataChunkSize);

};

} // namespace wav

#include "private/WriterImplementation.hpp"

#endif
