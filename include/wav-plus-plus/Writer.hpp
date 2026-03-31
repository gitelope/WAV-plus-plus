
#ifndef wavplusplus_Writer_hpp
#define wavplusplus_Writer_hpp

#include "Exceptions.hpp"
#include "Formats.hpp"
#include "private/int24.hpp"

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

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

    // Use to specify speaker positions in a multichannel file.
    // (Other ctor can be used to create a multichannel file with
    // unassigned speaker positions.)
    Writer (std::string const & filepath,
            ExportFormat exportFormat,
            uint32_t sampleRate,
            std::vector<SpeakerPosition> const & speakerPositions,
            size_t bufferSizeInKB = 32);

    ~Writer(); // Closes file if client does not.

    // write()
    // Write interleaved samples. Return the number of samples written.
    // May throw wav::WriteError.
    //
    template <FloatOrDouble T>
    size_t write (T const * source, size_t samplesToWrite);
    //
    template <FloatOrDouble T>
    size_t write (std::vector<T> const & source, size_t samplesToWrite);
    //
    template <FloatOrDouble T>
    size_t write (std::vector<T> const & source);

    // writeFrames
    // Write a number of frames. Return the number of frames written.
    // May throw wav::WriteError.
    //
    template <FloatOrDouble T>
    size_t writeFrames (T const * source, size_t framesToWrite);
    //
    template <FloatOrDouble T>
    size_t writeFrames (std::vector<T> const & source, size_t framesToWrite);
    //
    template <FloatOrDouble T>
    size_t writeFrames (std::vector<T> const & source);

    bool isOpen() const { return file != nullptr; }

    // Manual finalization allows client to handle exceptions.
    // Destructor will call if client doesn't, swallowing any exceptions.
    void Close();

private:

    std::unique_ptr<FILE, decltype(&fclose)> file;

    uint16_t channels;
    uint16_t bitsPerSample;
    uint16_t formatCode;
    uint32_t fmtChunkSize;
    uint32_t channelMask {0};

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

    void SharedInitialization (ExportFormat exportFormat, uint32_t sampleRate, size_t bufferSizeInKB);
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
