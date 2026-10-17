// WAV++
// Copyright (c) 2025 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_WriterImplementation_hpp
#define wavplusplus_WriterImplementation_hpp

#include "Byteswap.hpp"
#include "FileOffset.hpp"
#include "QuantDequantConstants.hpp"

#include <algorithm>
#include <bit>
#include <cassert>
#include <cmath>
#include <tuple>
#include <type_traits>

namespace wav
{

inline Writer::Writer (std::filesystem::path const & filepath,
                       ExportFormat exportFormat,
                       uint32_t sampleRate,
                       uint16_t channels_,
                       uint32_t channelMask_,
                       size_t bufferSizeInKB)
:file (fileForWriting (filepath))
,channels (channels_)
,channelMask (channelMask_)
{
    if (sampleRate == 0)
    {
        throw WriteError (WriteErrorCode::SampleRateZero, "Sample rate must be greater than zero.");
    }

    if (channels == 0)
    {
        throw WriteError (WriteErrorCode::ChannelsZero, "Channel count must be greater than zero.");
    }
    if (channels > maxChannels)
    {
        throw WriteError (WriteErrorCode::ChannelsExceedsMax, "Channel count exceeds the supported maximum of " + std::to_string (static_cast<unsigned> (maxChannels)) + ".");
    }

    if (channelMask & ~SpeakerPosition::ValidBits)
    {
        throw WriteError (WriteErrorCode::ChannelMaskInvalid, "Channel mask contains undefined speaker position bits.");
    }

    if (bufferSizeInKB == 0)
    {
        throw WriteError (WriteErrorCode::BufferSizeZero, "Write buffer size must be greater than zero.");
    }

    std::tie (this->bitsPerSample, this->formatCode) = bitDepthAndFormatCode (exportFormat);

    fmtChunkSize = (channels >= 3 || channelMask || bitsPerSample > 16)
        ? FormatChunkSize::WAVEFORMATEXTENSIBLE
        : FormatChunkSize::WAVEFORMATEX;

    // Write header with placeholders for RIFF chunk size and data chunk size.
    WriteHeader (sampleRate);

    InitializeBuffer (bufferSizeInKB * 1024);
}

template <FloatOrDouble T>
size_t Writer::write (T const * source, size_t samplesToWrite)
{
    if (!file || samplesToWrite == 0) return 0;

    // Check if writing would exceed WAV format limits.
    const uint64_t newTotalSamples {static_cast<uint64_t> (totalSamplesWritten) + samplesToWrite};
    const uint64_t newDataSize {newTotalSamples * (bitsPerSample / 8U)};
    if (newDataSize > maxDataChunkSize)
    {
        throw WriteError (WriteErrorCode::WriteWouldExceedWAVSizeLimit, "Writing would exceed WAV format data chunk size limit.");
    }

    if (formatCode == FormatCode::FloatPCM)
    {
        if constexpr (std::is_same_v<T, float>)
        {
            switch (bitsPerSample)
            {
                case 32: WriteFloatPCM_NoConversion (source, samplesToWrite);  break;
                case 64: WriteFloatToDouble         (source, samplesToWrite);  break;
                default: throw WriteError (WriteErrorCode::InternalError, "Unsupported float PCM bit depth: " + std::to_string (bitsPerSample));
            }
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            switch (bitsPerSample)
            {
                case 32: WriteDoubleToFloat         (source, samplesToWrite);  break;
                case 64: WriteFloatPCM_NoConversion (source, samplesToWrite);  break;
                default: throw WriteError (WriteErrorCode::InternalError, "Unsupported float PCM bit depth: " + std::to_string (bitsPerSample));
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
            default: throw WriteError (WriteErrorCode::InternalError, "Unsupported linear PCM bit depth: " + std::to_string (bitsPerSample));
        }
    }

    totalSamplesWritten += samplesToWrite;

    return samplesToWrite;
}

template <FloatOrDouble T>
size_t Writer::write (std::vector<T> const & source)
{
    return write (source.data(), source.size());
}

template <FloatOrDouble T>
size_t Writer::write (std::vector<T> const & source, size_t samplesToWrite)
{
    if (samplesToWrite > source.size())
    {
        throw WriteError (WriteErrorCode::SamplesToWriteExceedsSourceSize, "samples to write > source size");
    }
    return write (source.data(), samplesToWrite);
}

template <FloatOrDouble T>
size_t Writer::writeFrames (T const * source, size_t framesToWrite)
{
    return write (source, framesToWrite * channels) / channels;
}

template <FloatOrDouble T>
size_t Writer::writeFrames (std::vector<T> const & source)
{
    // check that there are a whole number of frames
    if (source.size() % channels != 0)
    {
        throw WriteError (WriteErrorCode::SourceFrameSizeIsFractional, "vector size is not a whole number of frames.");
    }
    return write (source.data(), source.size()) / channels;
}

template <FloatOrDouble T>
size_t Writer::writeFrames (std::vector<T> const & source, size_t framesToWrite)
{
    if (framesToWrite * channels > source.size())
    {
        throw WriteError (WriteErrorCode::FramesToWriteExceedsSourceSize, "frames to write exceeds the size of source");
    }
    return write (source.data(), framesToWrite * channels) / channels;
}

template <FloatOrDouble T>
void Writer::WriteFloatPCM_NoConversion (T const * source, size_t totalSamplesToWrite)
{
    if constexpr (std::endian::native == std::endian::little)
    {
        if (std::fwrite (source, sizeof (T), totalSamplesToWrite, file.get()) != totalSamplesToWrite)
        {
            throw WriteError (WriteErrorCode::FileWriteFailed, "Failed to write buffer to file.");
        }
    }
    else
    {
        if constexpr (std::is_same_v<T, float>)
        {
            auto * const buffer {bufferFloatPCM32.data()};
            const auto bufferSize {bufferFloatPCM32.size()};

            size_t samplesRemaining {totalSamplesToWrite};
            size_t readPosition {0};

            while (samplesRemaining)
            {
                const size_t samplesToWrite {std::min (samplesRemaining, bufferSize)};

                // Byteswap and copy to buffer.
                for (size_t i = 0; i < samplesToWrite; ++i)
                {
                    buffer [i] = wav::byteswap (source [readPosition + i]);
                }
                if (std::fwrite (buffer, 4, samplesToWrite, file.get()) != samplesToWrite)
                {
                    throw WriteError (WriteErrorCode::FileWriteFailed, "Failed to write buffer to file.");
                }

                readPosition += samplesToWrite;
                samplesRemaining -= samplesToWrite;
            }
        }
        else // double
        {
            auto * const buffer {bufferFloatPCM64.data()};
            const auto bufferSize {bufferFloatPCM64.size()};

            size_t samplesRemaining {totalSamplesToWrite};
            size_t readPosition {0};

            while (samplesRemaining)
            {
                const size_t samplesToWrite {std::min (samplesRemaining, bufferSize)};

                // Byteswap and copy to buffer.
                for (size_t i = 0; i < samplesToWrite; ++i)
                {
                    buffer [i] = wav::byteswap (source [readPosition + i]);
                }
                if (std::fwrite (buffer, 8, samplesToWrite, file.get()) != samplesToWrite)
                {
                    throw WriteError (WriteErrorCode::FileWriteFailed, "Failed to write buffer to file.");
                }

                readPosition += samplesToWrite;
                samplesRemaining -= samplesToWrite;
            }
        }
    }
}

inline void Writer::WriteFloatToDouble (float const * source, size_t totalSamplesToWrite)
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
                buffer [i] = wav::byteswap (buffer [i]);
            }
        }

        if (std::fwrite (buffer, bytesPerSample, samplesToWrite, file.get()) != samplesToWrite)
        {
            throw WriteError (WriteErrorCode::FileWriteFailed, "Failed to write buffer to file.");
        }

        readPosition += samplesToWrite;
        samplesRemaining -= samplesToWrite;
    }
}

inline void Writer::WriteDoubleToFloat (double const * source, size_t totalSamplesToWrite)
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
                buffer [i] = wav::byteswap (buffer [i]);
            }
        }

        if (std::fwrite (buffer, bytesPerSample, samplesToWrite, file.get()) != samplesToWrite)
        {
            throw WriteError (WriteErrorCode::FileWriteFailed, "Failed to write buffer to file.");
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

    constexpr double scale  {quantization::uint8Scale};
    constexpr double offset {quantization::uint8Offset};
    constexpr double min    {quantization::uint8Min};
    constexpr double max    {quantization::uint8Max};

    size_t samplesRemaining {totalSamplesToWrite};
    size_t readPosition {0};

    while (samplesRemaining)
    {
        const size_t samplesToWrite {std::min (samplesRemaining, bufferSize)};

        // Quantize.
        for (size_t i = 0; i < samplesToWrite; ++i)
        {
            const auto scaled {scale * static_cast<double> (source [readPosition + i])};
            /* dither */
            const auto rounded {std::round (scaled)};
            const auto shifted {rounded + offset};
            const auto clamped {std::clamp (shifted, min, max)};
            buffer [i] = static_cast<uint8_t> (clamped);
        }

        if (std::fwrite (buffer, bytesPerSample, samplesToWrite, file.get()) != samplesToWrite)
        {
            throw WriteError (WriteErrorCode::FileWriteFailed, "Failed to write buffer to file.");
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

    constexpr double scale {quantization::int16Scale};
    constexpr double min   {quantization::int16Min};
    constexpr double max   {quantization::int16Max};

    size_t samplesRemaining {totalSamplesToWrite};
    size_t readPosition {0};

    while (samplesRemaining)
    {
        const size_t samplesToWrite {std::min (samplesRemaining, bufferSize)};

        // Quantize.
        for (size_t i = 0; i < samplesToWrite; ++i)
        {
            buffer [i] = static_cast<int16_t> (quantized (static_cast<double> (source [readPosition + i]), scale, min, max));
        }

        // Byteswap if needed.
        if constexpr (std::endian::native == std::endian::big)
        {
            for (size_t i = 0; i < samplesToWrite; ++i)
            {
                buffer [i] = wav::byteswap (buffer [i]);
            }
        }

        if (std::fwrite (buffer, bytesPerSample, samplesToWrite, file.get()) != samplesToWrite)
        {
            throw WriteError (WriteErrorCode::FileWriteFailed, "Failed to write buffer to file.");
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

    constexpr double scale {quantization::int24Scale};
    constexpr double min   {quantization::int24Min};
    constexpr double max   {quantization::int24Max};

    size_t samplesRemaining {totalSamplesToWrite};
    size_t readPosition {0};

    while (samplesRemaining)
    {
        const size_t samplesToWrite {std::min (samplesRemaining, bufferSize)};

        // Quantize.
        for (size_t i = 0; i < samplesToWrite; ++i)
        {
            buffer [i] = static_cast<int24> (quantized (static_cast<double> (source [readPosition + i]), scale, min, max));
        }

        // Byteswap if needed.
        if constexpr (std::endian::native == std::endian::big)
        {
            for (size_t i = 0; i < samplesToWrite; ++i)
            {
                std::swap (buffer[i].bytes[0], buffer[i].bytes[2]);
            }
        }

        if (std::fwrite (buffer, bytesPerSample, samplesToWrite, file.get()) != samplesToWrite)
        {
            throw WriteError (WriteErrorCode::FileWriteFailed, "Failed to write buffer to file.");
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

    // Need to use double because floats can't represent all the 32-bit int values.
    constexpr double scale {quantization::int32Scale};
    constexpr double min   {quantization::int32Min};
    constexpr double max   {quantization::int32Max};

    size_t samplesRemaining {totalSamplesToWrite};
    size_t readPosition {0};

    while (samplesRemaining)
    {
        const size_t samplesToWrite {std::min (samplesRemaining, bufferSize)};

        // Quantize.
        for (size_t i = 0; i < samplesToWrite; ++i)
        {
            // Need to use double to quantize since float can't represent all the 32-bit values.
            buffer [i] = static_cast<int32_t> (quantized (static_cast<double> (source [readPosition + i]), scale, min, max));
        }

        // Byteswap if needed.
        if constexpr (std::endian::native == std::endian::big)
        {
            for (size_t i = 0; i < samplesToWrite; ++i)
            {
                buffer [i] = wav::byteswap (buffer [i]);
            }
        }

        if (std::fwrite (buffer, bytesPerSample, samplesToWrite, file.get()) != samplesToWrite)
        {
            throw WriteError (WriteErrorCode::FileWriteFailed, "Failed to write buffer to file.");
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

inline void WriteToHeader (std::FILE * out, std::string const & str)
{
    if (std::fwrite (str.data(), 1, str.size(), out) != str.size())
    {
        throw WriteError (WriteErrorCode::FileWriteFailed, "Failed to write string to file.");
    }
}

// Write integer as header field. (Would be a bit slow for writing data chunk.)
template <std::integral T>
void WriteToHeader (std::FILE * out, T value)
{
    std::array<std::byte, sizeof(T)> bytes;
    for (size_t i=0; i < bytes.size(); ++i)
    {
        bytes[i] = static_cast<std::byte> (value >> (8*i));
    }
    if (std::fwrite (bytes.data(), 1, bytes.size(), out) != bytes.size())
    {
        throw WriteError (WriteErrorCode::FileWriteFailed, "Failed to write to file.");
    }
}

inline void WriteToHeader (std::FILE * out, GUID const & guid)
{
    WriteToHeader (out, guid.data1);
    WriteToHeader (out, guid.data2);
    WriteToHeader (out, guid.data3);
    if (std::fwrite (guid.data4.data(), 1, guid.data4.size(), out) != guid.data4.size())
    {
        throw WriteError (WriteErrorCode::FileWriteFailed, "Failed to write GUID to file.");
    }
}

inline void Writer::WriteHeader (uint32_t sampleRate)
{
    if (!file) return;

    const auto bytesPerSample {bitsPerSample / 8U};
    const bool useExtensible {fmtChunkSize == FormatChunkSize::WAVEFORMATEXTENSIBLE};

    // blocks/sec * samples/block * bytes/sample = bytes/sec
    const auto bytesPerSecond {sampleRate * channels * bytesPerSample};

    // block size in bytes = samples/block * bytes/sample
    const auto blockAlign {static_cast<uint16_t> (channels * bytesPerSample)};

    // RIFF header
    WriteToHeader (file.get(), "RIFF");
    // Record position of RIFF chunk size so it can be updated later:
    riffChunkSizePosition = tell (file.get());
    if (riffChunkSizePosition == -1) throw WriteError (WriteErrorCode::FileTellFailed, "Failed to get RIFF chunk size position.");
    // Write RIFF chunk size placeholder:
    WriteToHeader (file.get(), static_cast<uint32_t> (0));
    WriteToHeader (file.get(), "WAVE");

    // fmt chunk — common to WAVEFORMATEX and WAVEFORMATEXTENSIBLE
    WriteToHeader (file.get(), "fmt ");
    WriteToHeader (file.get(), static_cast<uint32_t> (fmtChunkSize));
    WriteToHeader (file.get(), static_cast<uint16_t> (useExtensible ? FormatCode::Extensible : formatCode));
    WriteToHeader (file.get(), static_cast<uint16_t> (channels));
    WriteToHeader (file.get(), static_cast<uint32_t> (sampleRate));
    WriteToHeader (file.get(), static_cast<uint32_t> (bytesPerSecond));
    WriteToHeader (file.get(), static_cast<uint16_t> (blockAlign));
    WriteToHeader (file.get(), static_cast<uint16_t> (bitsPerSample));

    if (useExtensible)
    {
        // fmt - WAVEFORMATEXTENSIBLE-specific
        WriteToHeader (file.get(), static_cast<uint16_t> (22)); // cbSize
        WriteToHeader (file.get(), static_cast<uint16_t> (bitsPerSample)); // validBitsPerSample
        WriteToHeader (file.get(), static_cast<uint32_t> (channelMask));
        WriteToHeader (file.get(), formatCode == FormatCode::LinearPCM
                                   ? ExtensibleFormat::LinearPCM
                                   : ExtensibleFormat::FloatPCM); // subformat
    }
    else
    {
        // fmt - WAVEFORMATEX-specific
        WriteToHeader (file.get(), static_cast<uint16_t> (0)); // cbSize
    }

    // data chunk header
    WriteToHeader (file.get(), "data");
    // Record position of data chunk size so it can be updated later:
    dataChunkSizePosition = tell (file.get());
    if (dataChunkSizePosition == -1) throw WriteError (WriteErrorCode::FileTellFailed, "Failed to get data chunk size position.");
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
    // RIFF chunk size includes (everything after "RIFF" and its
    // 4-byte size field):
    //
    // "WAVE"             4 bytes
    //
    // "fmt "             4 bytes
    // format chunk size  4 bytes (WAVEFORMATEX: 18, WAVEFORMATEXTENSIBLE: 40)
    // format code        2 bytes
    // channels           2 bytes
    // sample rate        4 bytes
    // bytes per second   4 bytes
    // block align        2 bytes
    // bits per sample    2 bytes
    // cbSize             2 bytes (WAVEFORMATEX: 0, WAVEFORMATEXTENSIBLE: 22)
    //
    // "data"             4 bytes
    // data chunk size    4 bytes
    // [data]             <data chunk size> bytes
    // padding            <data chunk size> % 2 bytes (= 1 byte of padding if odd, else none)
    //
    // Letting
    // R = RIFF chunk size
    // D = data chunk size
    // F = format chunk size
    //
    // R = (20 + F) + D + (D % 2)
    //
    // Since the padding byte for odd sizes ensures that D + (D % 2)
    // is always even, we know that R must be even.
    // Therefore it can't be 2^32 - 1, but it could be 2^32 - 2,
    // which gives us the maximum value for D:
    // D + (D % 2) = 2^32 - 2 - 20 - F = 4,294,967,274 - F
    // D = 4,294,967,274 - F - (D % 2)
    // The largest D will be an even value:
    // Dmax = 4,294,967,274 - F
    //
    // WAVEFORMATEX         (F = 18): Dmax = 4,294,967,256
    // WAVEFORMATEXTENSIBLE (F = 40): Dmax = 4,294,967,234
    //
    // When we constrain the data chunk size so that the data chunk
    // contains a whole number of blocks, a.k.a. frames, we get
    // Dmax_blockAligned = floor (Dmax / blockAlign) * blockAlign
    // (Integer division gives us the floor for free.)
    maxDataChunkSize = ((4'294'967'274u - fmtChunkSize) / blockAlign) * blockAlign;
}

inline void Writer::Close()
{
    if (!file) return;

    // Flush any buffered data.
    if (std::fflush (file.get()) != 0)
    {
        throw WriteError (WriteErrorCode::FileFlushFailed, "Failed to flush file buffers.");
    }

    const auto dataChunkSize {static_cast<uint32_t>(totalSamplesWritten * (bitsPerSample / 8U))};

    // Add padding byte if there are an odd number of bytes in the data chunk.
    if (dataChunkSize % 2) WriteToHeader (file.get(), static_cast<uint8_t> (0));
    // Update header with data chunk size and RIFF chunk size.
    UpdateHeader (dataChunkSize);

    if (std::fclose (file.release()) != 0)
    {
        throw WriteError (WriteErrorCode::FileCloseFailed, "Failed to close file");
    }
}

inline Writer::~Writer()
{
    // Try to close the file if the user has not already.
    // (User can call Close() manually to surface errors.)
    if (file)
    {
        try
        {
            Close();
        }
        catch (...)
        {
            // Emit no exceptions to prevent the possibility of a fatal throw during stack unwinding.
            // (Perhaps error logging could be added.)
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
    if (seek (file.get(), dataChunkSizePosition, SEEK_SET) != 0)
    {
        throw WriteError (WriteErrorCode::FileSeekFailed, "Failed to reposition to data chunk size.");
    }
    WriteToHeader (file.get(), dataChunkSize);

    // Update RIFF chunk size, including padding if present.
    // See SetMaximumDataChunkSize() comments for derivation of:
    // R = (20 + F) + D + (D % 2)
    const uint32_t riffChunkSize {(20 + fmtChunkSize) + dataChunkSize + (dataChunkSize % 2)};
    if (seek (file.get(), riffChunkSizePosition, SEEK_SET) != 0)
    {
        throw WriteError (WriteErrorCode::FileSeekFailed, "Failed to reposition to RIFF chunk size");
    }
    WriteToHeader (file.get(), riffChunkSize);
}

} // namespace wav

#endif

