
#include "WriteHelpers.hpp"

namespace wav
{

void WriteHeader (FILE * out, uint16_t bitsPerSample, uint16_t formatCode, int sampleRate, int channels, size_t numSamples)
{
    const auto bytesPerSample {bitsPerSample / 8};
    constexpr auto formatChunkSize {FormatChunkSize::PCMWAVEFORMAT};

    // Note: data chunk size will not and should not account for a padding byte
    // when samples.size() is odd. However, that padding byte will and should be
    // reflected in RIFF chunk size.

    const auto dataChunkSize {static_cast<uint32_t> (numSamples /*samples.size()*/ * bytesPerSample)};
    const uint32_t riffChunkSize

    // RIFF chunk
    = 4 // "WAVE"

    // fmt  chunk
    + 4  // "fmt "
    + 4  // chunk size
    + formatChunkSize // in bytes

    // data chunk
    + 4 // "data"
    + 4 // chunk size
    + dataChunkSize // (in bytes) num samples x bytes per sample
    + dataChunkSize % 2; // 1 byte of padding when dataChunkSize is an odd number

    // blocks/sec * samples/block * bytes/sample = bytes/sec
    const auto bytesPerSecond {static_cast<uint32_t> (sampleRate * channels * bytesPerSample)};

    // block size in bytes = samples/block * bytes/sample
    const auto blockAlign {static_cast<uint16_t> (channels * bytesPerSample)};

    WriteToHeader (out, "RIFF");
    WriteToHeader (out, riffChunkSize);
    WriteToHeader (out, "WAVE");

    WriteToHeader (out, "fmt ");
    WriteToHeader (out, formatChunkSize);
    WriteToHeader (out, formatCode);
    WriteToHeader (out, static_cast<uint16_t> (channels));
    WriteToHeader (out, static_cast<uint32_t> (sampleRate)); // blocks (a.k.a. frames) per second
    WriteToHeader (out, bytesPerSecond);
    WriteToHeader (out, blockAlign);
    WriteToHeader (out, bitsPerSample);

    WriteToHeader (out, "data");
    WriteToHeader (out, dataChunkSize);
}

void WriteToHeader (FILE * out, std::string const & str)
{
    if (fwrite (str.data(), 1, str.size(), out) != str.size())
    {
        throw WriteError ("Failed to write string to file.");
    }
}

std::pair<uint16_t,uint16_t> bitDepthAndFormatCode (ExportFormat ef)
{
    switch (ef)
    {
        case ExportFormat::LinearPCM_8:  return {8,  FormatCode::LinearPCM};
        case ExportFormat::LinearPCM_16: return {16, FormatCode::LinearPCM};
        case ExportFormat::LinearPCM_24: return {24, FormatCode::LinearPCM};
        case ExportFormat::LinearPCM_32: return {32, FormatCode::LinearPCM};
        case ExportFormat::FloatPCM_32:  return {32, FormatCode::FloatPCM};
        case ExportFormat::FloatPCM_64:  return {64, FormatCode::FloatPCM};
    }
}

size_t bufferSizeInBytes (size_t requestedBufferSizeInBytes, size_t numSamples, uint16_t bitsPerSample)
{
    assert (bitsPerSample % 8 == 0);
    const auto sampleSizeInBytes {static_cast<size_t> (bitsPerSample / 8)};
    const auto totalBytesToWrite {numSamples * sampleSizeInBytes};

    // Determine buffer size from requested size, sample size, and number of samples:
    // 1. Buffer must be able to hold an entire sample's worth of bytes.
    size_t bytesPerBuffer = std::max (requestedBufferSizeInBytes, sampleSizeInBytes);
    // 2. Buffer size must correspond to a whole number of samples.
    bytesPerBuffer = (bytesPerBuffer / sampleSizeInBytes) * sampleSizeInBytes;
    // 3. Buffer size must not exceed the total number of bytes to write.
    bytesPerBuffer = std::min (bytesPerBuffer, totalBytesToWrite);

    return bytesPerBuffer;
}

} // namespace wav
