
#ifndef wavplusplus_Writer_hpp
#define wavplusplus_Writer_hpp

#include "Formats.hpp"
#include "QuantizationConstants.hpp"
#include "BufferSizing.hpp"
#include "int24.hpp"
#include "Exceptions.hpp"

#include <cassert>
#include <vector>
#include <cstdio>
#include <bit>
#include <string>
#include <algorithm>
#include <memory>

static_assert (sizeof (float) == 4);
static_assert (sizeof (double) == 8);
static_assert (CHAR_BIT == 8);

namespace wav
{

/*

 sample encoding options:
 linear PCM: 8-bit, 16-bit, 24-bit, and 32-bit
 float PCM: 32-bit and 64-bit

 channel options:
 mono, stereo, and multichannel

 format:
 PCMWAVEFORMAT (very common, but technically non-standard for some of the above options)

 ---

 quantization (linear PCM only):

 double or float -> signed int (16, 24 and 32 bits):
 1. Multiply by 2^(bits-1) - 1
 (e.g. 32,767 for 16-bit linear PCM)
 2. Round to nearest integer.*
 3. Clamp to [1 - 2^(bits-1), 2^(bits-1) - 1]
 (e.g. [-32,767, 32,767] for 16-bit linear PCM)

 double or float -> unsigned 8-bit int:
 1. Multiply by 127
 2. Add 128.
 3. Round to nearest integer.*
 4. Clamp to [1, 255].

 * Optional TPDF dither (not implemented) would come before rounding.

 */

class Writer
{

public:

    explicit Writer (std::string const & filepath,
                     ExportFormat exportFormat,
                     uint32_t sampleRate,
                     uint16_t channels,
                     size_t bufferSizeInKB = 32)
    :file (fopen (filepath.c_str(), "wb"), fclose)
    ,channels (channels)
    {
        if (!file)
        {
            throw WriteError ("Failed to open file for writing: " + filepath);
        }

        std::tie (this->bitsPerSample, this->formatCode) = bitDepthAndFormatCode (exportFormat);

        // Write header with placeholders for RIFF chunk size and data chunk size.
        WriteHeader (sampleRate);

        InitializeBuffer (bufferSizeInKB * 1024);
    }

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

template <FloatOrDouble T>
size_t Writer::write (T const * source, size_t samplesToWrite)
{
    if (!file || samplesToWrite == 0) return 0;

    // Check if writing would exceed WAV format limits.
    const uint64_t newTotalSamples {static_cast<uint64_t> (totalSamplesWritten) + samplesToWrite};
    const uint64_t newDataSize {newTotalSamples * (bitsPerSample / 8U)};
    if (newDataSize > maxDataChunkSize)
    {
        throw WriteError ("Writing would exceed WAV format data chunk size limit.");
    }

    if (formatCode == FormatCode::FloatPCM)
    {
        if constexpr (std::is_same_v<T, float>)
        {
            switch (bitsPerSample)
            {
                case 32: WriteFloatPCM_NoConversion (source, samplesToWrite);  break;
                case 64: WriteFloatToDouble         (source, samplesToWrite);  break;
                default: throw WriteError ("Unsupported float PCM bit depth: " + std::to_string (bitsPerSample));
            }
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            switch (bitsPerSample)
            {
                case 32: WriteDoubleToFloat         (source, samplesToWrite);  break;
                case 64: WriteFloatPCM_NoConversion (source, samplesToWrite);  break;
                default: throw WriteError ("Unsupported float PCM bit depth: " + std::to_string (bitsPerSample));
            }
        }
    }
    else if (formatCode == FormatCode::LinearPCM)
    {
        switch (bitsPerSample)
        {
            case 8:  WriteLPCMU8 (source, samplesToWrite); break;
            case 16: WriteLPCM16 (source, samplesToWrite); break;
            case 24: WriteLPCM24 (source, samplesToWrite); break;
            case 32: WriteLPCM32 (source, samplesToWrite); break;
            default: throw WriteError ("Unsupported linear PCM bit depth: " + std::to_string (bitsPerSample));
        }
    }

    totalSamplesWritten += samplesToWrite;

    return samplesToWrite;
}

// fixed-size array
template <FloatOrDouble T, size_t N>
size_t Writer::write (const T (&source)[N])
{
    return write (source, N);
}

// std::vector
template <FloatOrDouble T>
size_t Writer::write (std::vector<T> const & source)
{
    return write (source.data(), source.size());
}

// std::vector
template <FloatOrDouble T>
size_t Writer::write (std::vector<T> const & source, size_t samplesToWrite)
{
    return write (source.data(), std::min (samplesToWrite, source.size()));
}

template <FloatOrDouble T, size_t N>
size_t Writer::write (std::array<T,N> const & source)
{
    return write (source.data(), N);
}

template <FloatOrDouble T, size_t N>
size_t Writer::write (std::array<T,N> const & source, size_t samplesToWrite)
{
    return write (source.data(), std::min (samplesToWrite, N));
}

template <FloatOrDouble T>
size_t Writer::writeFrames (T const * source, size_t framesToWrite)
{
    return write (source, framesToWrite * channels) / channels;
}

template <FloatOrDouble T>
void Writer::WriteFloatPCM_NoConversion (T const * source, size_t samplesToWrite)
{
    if constexpr (std::endian::native == std::endian::little)
    {
        if (fwrite (source, sizeof (T), samplesToWrite, file.get()) != samplesToWrite)
        {
            throw WriteError ("Failed to write buffer to file.");
        }
    }
    else
    {
        if constexpr (std::is_same_v<T, float>)
        {
            auto * const buffer {bufferFloatPCM32.data()};

            // Byteswap and copy to buffer.
            for (size_t i = 0; i < samplesToWrite; ++i)
            {
                buffer [i] = std::bit_cast<float> (std::byteswap (std::bit_cast<uint32_t> (source[i])));
            }
            if (fwrite (buffer, 4, samplesToWrite, file.get()) != samplesToWrite)
            {
                throw WriteError ("Failed to write buffer to file.");
            }
        }
        else // double
        {
            auto * const buffer {bufferFloatPCM64.data()};

            // Byteswap and copy to buffer.
            for (size_t i = 0; i < samplesToWrite; ++i)
            {
                buffer [i] = std::bit_cast<double> (std::byteswap (std::bit_cast<uint64_t> (source[i])));
            }
            if (fwrite (buffer, 8, samplesToWrite, file.get()) != samplesToWrite)
            {
                throw WriteError ("Failed to write buffer to file.");
            }
        }
    }
}

void Writer::WriteFloatToDouble (float const * source, size_t totalSamplesToWrite)
{
    auto * const buffer {bufferFloatPCM64.data()};
    const auto bufferSize {bufferFloatPCM64.size()};

    constexpr size_t bytesPerSample {8};

    size_t samplesRemaining {totalSamplesToWrite};
    size_t readPosition {0};

    while (samplesRemaining)
    {
        const size_t samplesToWrite {std::min (samplesRemaining, bufferSize)};

        // Convert to double.
        for (size_t i = 0; i < samplesToWrite; ++i)
        {
            buffer [i] = static_cast<double> (source [readPosition + i]);
        }

        // Byteswap if needed.
        if constexpr (std::endian::native == std::endian::big)
        {
            for (size_t i = 0; i < samplesToWrite; ++i)
            {
                buffer [i] = std::bit_cast<double> (std::byteswap (std::bit_cast<uint64_t> (buffer [i])));
            }
        }

        if (fwrite (buffer, bytesPerSample, samplesToWrite, file.get()) != samplesToWrite)
        {
            throw WriteError ("Failed to write buffer to file.");
        }

        readPosition += samplesToWrite;
        samplesRemaining -= samplesToWrite;
    }
}

void Writer::WriteDoubleToFloat (double const * source, size_t totalSamplesToWrite)
{
    auto * const buffer {bufferFloatPCM32.data()};
    const auto bufferSize {bufferFloatPCM32.size()};

    constexpr size_t bytesPerSample {4};

    size_t samplesRemaining {totalSamplesToWrite};
    size_t readPosition {0};

    while (samplesRemaining)
    {
        const size_t samplesToWrite {std::min (samplesRemaining, bufferSize)};

        // Convert to float.
        for (size_t i = 0; i < samplesToWrite; ++i)
        {
            buffer [i] = static_cast<float> (source [readPosition + i]);
        }

        // Byteswap if needed.
        if constexpr (std::endian::native == std::endian::big)
        {
            for (size_t i = 0; i < samplesToWrite; ++i)
            {
                buffer [i] = std::bit_cast<float> (std::byteswap (std::bit_cast<uint32_t> (buffer [i])));
            }
        }

        if (fwrite (buffer, bytesPerSample, samplesToWrite, file.get()) != samplesToWrite)
        {
            throw WriteError ("Failed to write buffer to file.");
        }

        readPosition += samplesToWrite;
        samplesRemaining -= samplesToWrite;
    }
}

template <FloatOrDouble T>
void Writer::WriteLPCMU8 (T const * source, size_t totalSamplesToWrite)
{
    // local data pointer helps the optimizer
    auto * const buffer {bufferLPCMU8.data()};
    const auto bufferSize {bufferLPCMU8.size()};
    constexpr size_t bytesPerSample {1};

    constexpr auto scale  {static_cast<T> (uint8Scale)};
    constexpr auto offset {static_cast<T> (uint8QuantizationOffset)};
    constexpr auto min    {static_cast<T> (uint8Min)};
    constexpr auto max    {static_cast<T> (uint8Max)};

    size_t samplesRemaining {totalSamplesToWrite};
    size_t readPosition {0};

    while (samplesRemaining)
    {
        const size_t samplesToWrite {std::min (samplesRemaining, bufferSize)};

        // Quantize.
        for (size_t i = 0; i < samplesToWrite; ++i)
        {
            const auto scaled {scale * source [readPosition + i]};
            /* dither */
            const auto rounded {std::round (scaled)};
            const auto shifted {rounded + offset};
            const auto clamped {std::clamp (shifted, min, max)};
            buffer [i] = static_cast<uint8_t> (clamped);
        }

        if (fwrite (buffer, bytesPerSample, samplesToWrite, file.get()) != samplesToWrite)
        {
            throw WriteError ("Failed to write buffer to file.");
        }

        readPosition += samplesToWrite;
        samplesRemaining -= samplesToWrite;
    }
}


// quantize floating-point to signed integer (still in the form of a float or double).
template <FloatOrDouble T> T quantized (T value, T scale, T min, T max)
{
    const T scaled {scale * value};
    /* dither */
    const T rounded {std::round (scaled)};
    return std::clamp (rounded, min, max);
}

template <FloatOrDouble T>
void Writer::WriteLPCM16 (T const * source, size_t totalSamplesToWrite)
{
    auto * const buffer {bufferLPCM16.data()};
    const auto bufferSize {bufferLPCM16.size()};
    constexpr size_t bytesPerSample {2};

    constexpr auto scale {static_cast<T> (int16Scale)};
    constexpr auto min   {static_cast<T> (int16Min)};
    constexpr auto max   {static_cast<T> (int16Max)};

    size_t samplesRemaining {totalSamplesToWrite};
    size_t readPosition {0};

    while (samplesRemaining)
    {
        const size_t samplesToWrite {std::min (samplesRemaining, bufferSize)};

        // Quantize.
        for (size_t i = 0; i < samplesToWrite; ++i)
        {
            buffer [i] = static_cast<int16_t> (quantized (source [readPosition + i], scale, min, max));
        }

        // Byteswap if needed.
        if constexpr (std::endian::native == std::endian::big)
        {
            for (size_t i = 0; i < samplesToWrite; ++i)
            {
                buffer [i] = std::byteswap (buffer [i]);
            }
        }

        if (fwrite (buffer, bytesPerSample, samplesToWrite, file.get()) != samplesToWrite)
        {
            throw WriteError ("Failed to write buffer to file.");
        }

        readPosition += samplesToWrite;
        samplesRemaining -= samplesToWrite;
    }
}

template <FloatOrDouble T>
void Writer::WriteLPCM24 (T const * source, size_t totalSamplesToWrite)
{
    // local data pointer helps the optimizer
    auto * const buffer {bufferLPCM24.data()};
    const auto bufferSize {bufferLPCM24.size()};
    constexpr size_t bytesPerSample {3};

    constexpr auto scale {static_cast<T> (int24Scale)};
    constexpr auto min   {static_cast<T> (int24Min)};
    constexpr auto max   {static_cast<T> (int24Max)};

    size_t samplesRemaining {totalSamplesToWrite};
    size_t readPosition {0};

    while (samplesRemaining)
    {
        const size_t samplesToWrite {std::min (samplesRemaining, bufferSize)};

        // Quantize.
        for (size_t i = 0; i < samplesToWrite; ++i)
        {
            buffer [i] = static_cast<int24> (quantized (source [readPosition + i], scale, min, max));
        }

        // Byteswap if needed.
        if constexpr (std::endian::native == std::endian::big)
        {
            for (size_t i = 0; i < samplesToWrite; ++i)
            {
                std::swap (buffer[i].bytes[0], buffer[i].bytes[2]);
            }
        }

        if (fwrite (buffer, bytesPerSample, samplesToWrite, file.get()) != samplesToWrite)
        {
            throw WriteError ("Failed to write buffer to file.");
        }

        readPosition += samplesToWrite;
        samplesRemaining -= samplesToWrite;
    }
}

template <FloatOrDouble T>
void Writer::WriteLPCM32 (T const * source, size_t totalSamplesToWrite)
{
    auto * const buffer {bufferLPCM32.data()};
    const auto bufferSize {bufferLPCM32.size()};
    constexpr size_t bytesPerSample {4};

    constexpr auto scale {static_cast<T> (int32Scale)};
    constexpr auto min   {static_cast<T> (int32Min)};
    constexpr auto max   {static_cast<T> (int32Max)};

    size_t samplesRemaining {totalSamplesToWrite};
    size_t readPosition {0};

    while (samplesRemaining)
    {
        const size_t samplesToWrite {std::min (samplesRemaining, bufferSize)};

        // Quantize.
        for (size_t i = 0; i < samplesToWrite; ++i)
        {
            buffer [i] = static_cast<int32_t> (quantized (source [readPosition + i], scale, min, max));
        }

        // Byteswap if needed.
        if constexpr (std::endian::native == std::endian::big)
        {
            for (size_t i = 0; i < samplesToWrite; ++i)
            {
                buffer [i] = std::byteswap (buffer [i]);
            }
        }

        if (fwrite (buffer, bytesPerSample, samplesToWrite, file.get()) != samplesToWrite)
        {
            throw WriteError ("Failed to write buffer to file.");
        }

        readPosition += samplesToWrite;
        samplesRemaining -= samplesToWrite;
    }
}

inline void Writer::InitializeBuffer (size_t sizeInBytes)
{

    // Initialize the buffer needed for the current file format.
    if (formatCode == FormatCode::LinearPCM)
    {
        switch (bitsPerSample)
        {
            case 8:  bufferLPCMU8.resize (sizeInBytes);     break;
            case 16: bufferLPCM16.resize (sizeInBytes / 2); break;
            case 24: bufferLPCM24.resize (sizeInBytes / 3); break;
            case 32: bufferLPCM32.resize (sizeInBytes / 4); break;
        }
    }
    else if (formatCode == FormatCode::FloatPCM)
    {
        // We won't need a buffer on LE systems if the encoding format
        // matches the decoding format, i.e. double -> double or
        // float -> float, but we don't yet know what the decoding format
        // will be, so we always initialize a buffer just in case.
        if (bitsPerSample == 32)
        {
            // double -> float
            bufferFloatPCM32.resize (sizeInBytes / 4);
        }
        else if (bitsPerSample == 64)
        {
            // float -> double
            bufferFloatPCM64.resize (sizeInBytes / 8);
        }
    }
}

inline void WriteToHeader (FILE * out, std::string const & str)
{
    if (fwrite (str.data(), 1, str.size(), out) != str.size())
    {
        throw WriteError ("Failed to write string to file.");
    }
}

// Write integer as header field. (Would be a bit slow for writing data chunk.)
template <std::integral T>
void WriteToHeader (FILE * out, T value)
{
    std::array<std::byte, sizeof(T)> bytes;
    for (size_t i=0; i < bytes.size(); ++i)
    {
        bytes[i] = static_cast<std::byte> (value >> (8*i));
    }
    if (fwrite (bytes.data(), 1, bytes.size(), out) != bytes.size())
    {
        throw WriteError ("Failed to write to file.");
    }
}

inline void Writer::WriteHeader (uint32_t sampleRate)
{
    if (!file) return;

    const auto bytesPerSample {bitsPerSample / 8U};
    constexpr auto formatChunkSize {FormatChunkSize::PCMWAVEFORMAT};

    // blocks/sec * samples/block * bytes/sample = bytes/sec
    const auto bytesPerSecond {sampleRate * channels * bytesPerSample};

    // block size in bytes = samples/block * bytes/sample
    const auto blockAlign {static_cast<uint16_t> (channels * bytesPerSample)};

    // RIFF header
    WriteToHeader (file.get(), "RIFF");
    // Record position of RIFF chunk size so it can be updated later:
    riffChunkSizePosition = ftell (file.get());
    if (riffChunkSizePosition == -1) throw WriteError ("Failed to get RIFF chunk size position.");
    // Write RIFF chunk size placeholder:
    WriteToHeader (file.get(), static_cast<uint32_t> (0));
    WriteToHeader (file.get(), "WAVE");

    // fmt chunk
    WriteToHeader (file.get(), "fmt ");
    WriteToHeader (file.get(), static_cast<uint32_t> (formatChunkSize));
    WriteToHeader (file.get(), static_cast<uint16_t> (formatCode));
    WriteToHeader (file.get(), static_cast<uint16_t> (channels));
    WriteToHeader (file.get(), static_cast<uint32_t> (sampleRate));
    WriteToHeader (file.get(), static_cast<uint32_t> (bytesPerSecond));
    WriteToHeader (file.get(), static_cast<uint16_t> (blockAlign));
    WriteToHeader (file.get(), static_cast<uint16_t> (bitsPerSample));

    // data chunk header
    WriteToHeader (file.get(), "data");
    // Record position of data chunk size so it can be updated later:
    dataChunkSizePosition = ftell (file.get());
    if (dataChunkSizePosition == -1) throw WriteError ("Failed to get data chunk size position.");
    // Write data chunk size placeholder:
    WriteToHeader (file.get(), static_cast<uint32_t> (0)); // Will be updated in UpdateHeader()

    SetMaximumDataChunkSize (blockAlign);
}

inline void Writer::SetMaximumDataChunkSize (uint16_t blockAlign)
{
    // Calculate maximum data chunk size
    // with the requirement that only full frames, a.k.a.
    // blocks will be stored. No partial frames allowed.
    //
    // RIFF chunk size is stored in an unsigned 32-bit integer.
    // The maximum value can therefore be no more than 2^32 - 1
    // = 4,294,967,295.
    //
    // RIFF chunk size includes:
    //
    // "WAVE" - 4 bytes
    // ---
    // "fmt " - 4 bytes
    // format chunk size - 4 bytes
    // format code - 2 bytes
    // channels - 2 bytes
    // sample rate - 4 bytes
    // bytes per second - 4 bytes
    // block align - 2 bytes
    // bits per sample - 2 bytes
    // ---
    // "data" - 4 bytes
    // data chunk size - 4 bytes
    // data - bytes = data chunk size
    // 1 padding byte if data is odd number of bytes
    //
    // Lettting
    // R = RIFF chunk size
    // D = data chunk size
    // we can express RIFF chunk size as a function of
    // data chunk size:
    //
    // R = 36 + D + (D % 2)
    //
    // Since the padding byte for odd sizes ensures that D + (D % 2)
    // is always even, we know that R must be even.
    // Therefore it can't be 2^32 - 1, but it could be 2^32 - 2,
    // which gives us the maximum value for D:
    // D + (D % 2) = 2^32 - 2 - 36 = 2^32 - 38 = 4,294,967,258
    // D = 4,294,967,258 - (D % 2)
    // The largest D will be an even value:
    // Dmax = 4,294,967,258
    //
    // When we constrain the data chunk size so that the data chunk contains
    // whole number of blocks, a.k.a. frames, we get
    // Dmax_blockAligned = floor (Dmax / blockSize) * blockSize
    // = floor (4,294,967,258 / blockSize) * blockSize
    // (Integer division gives us the floor for free.)
    maxDataChunkSize = (4'294'967'258 / blockAlign) * blockAlign;
}

inline void Writer::Close()
{
    if (!file) return;

    // Flush any buffered data.
    if (fflush (file.get()) != 0)
    {
        throw WriteError ("Failed to flush file buffers.");
    }

    const auto dataChunkSize {static_cast<uint32_t>(totalSamplesWritten * (bitsPerSample / 8U))};

    // Add padding byte if there are an odd number of bytes in the data chunk.
    if (dataChunkSize % 2) WriteToHeader (file.get(), static_cast<uint8_t> (0));
    // Update header with data chunk size and RIFF chunk size.
    UpdateHeader (dataChunkSize);

    if (fclose (file.release()) != 0)
    {
        throw WriteError ("Failed to close file");
    }
}

inline Writer::~Writer()
{
    // Try to close the file if the user has not already.
    if (file)
    {
        try
        {
            Close();
        }
        catch (...)
        {
            // Emit no exceptions.
            // Perhaps error logging could be added.
        }
    }
}

/// Updates data chunk size and RIFF chunk size, which is derived from it.
/// Note: The data chunk size should not include a data padding byte
/// (if it exists) while the RIFF chunk size should include all padding
/// bytes for all chunks in the file.
inline void Writer::UpdateHeader (uint32_t dataChunkSize)
{
    if (!file) return;

    // Update data chunk size. (Padding does not go into the count.)
    if (fseek (file.get(), dataChunkSizePosition, SEEK_SET) != 0)
    {
        throw WriteError ("Failed to reposition to data chunk size.");
    }
    WriteToHeader (file.get(), dataChunkSize);

    // Update RIFF chunk size, including padding if present.
    // See WriteHeader() comments for derivation of:
    // R = 36 + D + (D % 2)
    const uint32_t riffChunkSize {36 + dataChunkSize + (dataChunkSize % 2)};
    if (fseek (file.get(), riffChunkSizePosition, SEEK_SET) != 0)
    {
        throw WriteError ("Failed to reposition to RIFF chunk size");
    }
    WriteToHeader (file.get(), riffChunkSize);
}

} // namespace wav

#endif
