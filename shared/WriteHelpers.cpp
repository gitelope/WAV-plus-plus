
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

    WriteToFile (out, "RIFF");
    WriteToFile (out, riffChunkSize);
    WriteToFile (out, "WAVE");

    WriteToFile (out, "fmt ");
    WriteToFile (out, formatChunkSize);
    WriteToFile (out, formatCode);
    WriteToFile (out, static_cast<uint16_t> (channels));
    WriteToFile (out, static_cast<uint32_t> (sampleRate)); // blocks (a.k.a. frames) per second
    WriteToFile (out, bytesPerSecond);
    WriteToFile (out, blockAlign);
    WriteToFile (out, bitsPerSample);

    WriteToFile (out, "data");
    WriteToFile (out, dataChunkSize);
}

void WriteToFile (FILE * out, std::string const & str)
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

} // name space wav
