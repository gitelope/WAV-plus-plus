// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_ChunkInfo_hpp
#define wavplusplus_ChunkInfo_hpp

#include "Exceptions.hpp"
#include "Formats.hpp"

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <ostream>
#include <string>
#include <vector>

namespace wav
{

struct RiffChunk
{
    std::string id {"RIFF"};
    uint32_t size {0}; // = file size in bytes - 8
    std::string format {"WAVE"};
};

struct FormatChunk
{
    // WAVEFORMAT, PCMWAVEFORMAT, WAVEFORMATEX, WAVEFORMATEXTENSIBLE:
    std::string id {"fmt "};
    uint32_t size {0};
    uint16_t code {0}; // linear PCM (0x0001), float PCM (0x0003), or extensible (0xFFFE)
    uint16_t channels {0};
    uint32_t sampleRate {0};
    uint32_t byteRate {0};
    uint16_t blockAlign {0};

    // PCMWAVEFORMAT, WAVEFORMATEX, WAVEFORMATEXTENSIBLE:
    uint16_t bitsPerSample {0};

    // WAVEFORMATEX, WAVEFORMATEXTENSIBLE:
    uint16_t cbSize {0}; // extra format information size

    // WAVEFORMATEXTENSIBLE:
    uint16_t validBitsPerSample {0};
    uint32_t channelMask {0};
    GUID subFormat;
    std::string channelString () const;
};

struct DataChunk
{
    std::string id {"data"};
    uint32_t size {0};
    // first byte of first sample,
    // (-1 means data chunk hasn't been encountered yet):
    std::int64_t offset {-1};
};

struct FactChunk
{
    std::string id {"fact"};
    uint32_t size {0};
    uint32_t fileSize {0};
    bool present {false}; // false if not present or size != 4
};

struct Peak
{
    float value {0}; // signed. 1.0 represents 0dB for non-floating-point encodings.
    uint32_t positionInFrames {0};
};

struct PeakChunk
{
    std::string id {"PEAK"};
    uint32_t size {0};
    uint32_t version {0};
    uint32_t timestamp {0};
    std::vector<Peak> peaks;
    bool present {false}; // false: either not present or not valid
};

/// Reads in all the info from RIFF, fmt, data (except for the samples),
/// fact (optional), and PEAK (optional) chunks. The presence of other
/// chunks is recorded, but their contents are not read.

// WAV format notes:
//
// The whole file is a single RIFF chunk; fmt, data, fact, PEAK, and any
// other chunks (e.g. JUNK) are subchunks within it.
//
// The fmt chunk must precede the data chunk. The data chunk need not be last.
//
// Each subchunk's payload is padded to an even byte boundary; the pad
// byte is not counted in the subchunk's size field.
class ChunkInfo
{
public:

    ChunkInfo (std::filesystem::path const & filepath); // opens (and closes) file
    ChunkInfo (std::FILE * in);                         // construct from open file

    std::string summary() const;

    RiffChunk riff;
    FormatChunk format;
    DataChunk data;
    FactChunk fact;
    PeakChunk peak;

    std::vector<std::string> chunksEncountered;
    std::vector<std::string> chunksSkipped;

private:

    void ReadFile              (std::FILE * in);
    void ReadRiffChunk         (std::FILE * in);
    void ReadFormatChunk       (std::FILE * in);
    void ReadFactChunk         (std::FILE * in, std::int64_t chunkStart);
    void ReadPeakChunk         (std::FILE * in, std::int64_t chunkStart);
    void ReadDataChunkSizeAndSkipSamples (std::FILE * in);

    void RegisterChunk        (std::string const & id, std::int64_t position);
    void RegisterSkippedChunk (std::string const & id, std::int64_t position);

    void ValidateAfterParsing();

    bool fmtChunkEncountered {false};
    bool dataChunkEncountered {false};
    bool factChunkEncountered {false};
    bool peakChunkEncountered {false};

    std::int64_t actualFileSize {0};
};

std::ostream & operator<< (std::ostream &, ChunkInfo const &);
std::ostream & operator<< (std::ostream &, RiffChunk const &);
std::ostream & operator<< (std::ostream &, FormatChunk const &);
std::ostream & operator<< (std::ostream &, FactChunk const &);
std::ostream & operator<< (std::ostream &, PeakChunk const &);
std::ostream & operator<< (std::ostream &, DataChunk const &);

} // namespace wav

#include "private/ChunkInfoImplementation.hpp"

#endif
