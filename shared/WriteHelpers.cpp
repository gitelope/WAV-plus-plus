
#include "WriteHelpers.hpp"

namespace wav
{

void WriteHeader (FILE * out, uint16_t bitsPerSample, uint16_t formatCode, uint32_t sampleRate, uint16_t channels, size_t numSamples)
{
    const auto bytesPerSample {bitsPerSample / 8U};
    constexpr auto formatChunkSize {FormatChunkSize::PCMWAVEFORMAT};

    // Note: data chunk size will not and should not account for a padding byte
    // when samples.size() is odd. However, that padding byte will and should be
    // reflected in RIFF chunk size.

    const auto dataChunkSize {static_cast<uint32_t> (numSamples * bytesPerSample)};
    const uint32_t riffChunkSize

    // RIFF chunk
    = 4 // "WAVE"

    // fmt  chunk
    + 4  // "fmt "
    + 4  // chunk size
    + formatChunkSize // in bytes
    + formatChunkSize % 2 // 1 byte of padding when formatChunkSize is an odd number

    // data chunk
    + 4 // "data"
    + 4 // chunk size
    + dataChunkSize // (in bytes) num samples x bytes per sample
    + dataChunkSize % 2; // 1 byte of padding when dataChunkSize is an odd number

    // blocks/sec * samples/block * bytes/sample = bytes/sec
    const auto bytesPerSecond {sampleRate * channels * bytesPerSample};

    // block size in bytes = samples/block * bytes/sample
    const auto blockAlign {channels * bytesPerSample};

    // The explicit casts below ensure the right number of bytes for the format.

    WriteToHeader (out, "RIFF");
    WriteToHeader (out, static_cast<uint32_t> (riffChunkSize));
    WriteToHeader (out, "WAVE");

    WriteToHeader (out, "fmt ");
    WriteToHeader (out, static_cast<uint32_t> (formatChunkSize));
    WriteToHeader (out, static_cast<uint16_t> (formatCode));
    WriteToHeader (out, static_cast<uint16_t> (channels));
    WriteToHeader (out, static_cast<uint32_t> (sampleRate)); // blocks (a.k.a. frames) per second
    WriteToHeader (out, static_cast<uint32_t> (bytesPerSecond));
    WriteToHeader (out, static_cast<uint16_t> (blockAlign));
    WriteToHeader (out, static_cast<uint16_t> (bitsPerSample));

    WriteToHeader (out, "data");
    WriteToHeader (out, static_cast<uint32_t> (dataChunkSize));
}

void WriteToHeader (FILE * out, std::string const & str)
{
    if (fwrite (str.data(), 1, str.size(), out) != str.size())
    {
        throw WriteError ("Failed to write string to file.");
    }
}

} // namespace wav
