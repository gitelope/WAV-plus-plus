
#ifndef wavplusplus_Read_hpp
#define wavplusplus_Read_hpp

#include "ChunkInfo.hpp"
#include <string>
#include <vector>

namespace wav
{

// May throw wav::ReadError.
void Read (std::string const & filepath, std::vector<double> & destination, struct Info &, size_t requestedBufferSize = 32768);

struct Info
{
    int channels {0};
    int sampleRate {0};
    // total samples = channels * frames
    // length in seconds = frames / sampleRate
    int frames {0};
    int bitDepth {0}; // bits per sample
    int containerSizeInBits {0}; // sample containerSize >= sample bitDepth
    bool isLinearPCM {false};
    bool isFloatPCM {false};

    void ExtractFrom (ChunkInfo const &);
};
std::ostream & operator<< (std::ostream & os, Info const &);


//////////////////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
// (may throw wav::ReadError)

// Reads in a buffer of bytes (of adjustable size), decodes, and repeats until done.
// Likely to perform the fastest. Called by Read().
void ReadDataBuffered      (std::string const & filepath,
                            std::vector<double> & destination,
                            Info const & info,
                            DataChunk const & dataChunk,
                            size_t requestedBufferSizeInBytes);

// Reads in all bytes (into a buffer the size of the data) before decoding.
void ReadDataLargestBuffer (std::string const & filepath,
                            std::vector<double> & destination,
                            Info const & info,
                            DataChunk const & dataChunk);

// Used by ReadDataBuffered.
void ReadLinearPCM (std::vector<std::byte> const & buffer,
                    std::vector<double> & destination,
                    Info const & info,
                    size_t samplesToRead,
                    size_t writePosition);

// Used by ReadDataLargestBuffer.
void ReadLinearPCM (std::vector<std::byte> const & bytes,
                    std::vector<double> & destination,
                    Info const & info);

// Used by ReadDataBuffered.
void ReadFloatPCM  (std::vector<std::byte> const & buffer,
                    std::vector<double> & destination,
                    Info const & info,
                    size_t samplesToRead,
                    size_t writePosition);

// Used by ReadDataLargestBuffer.
void ReadFloatPCM  (std::vector<std::byte> const & bytes,
                    std::vector<double> & destination,
                    Info const & info);
//////////////////////////////////////////////////////////////////////////


} // namespace wav

#endif
