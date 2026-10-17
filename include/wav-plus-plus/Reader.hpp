// WAV++
// Copyright (c) 2025 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_Reader_hpp
#define wavplusplus_Reader_hpp

#include "Exceptions.hpp"
#include "Formats.hpp"
#include "FileInfo.hpp"
#include "ChunkInfo.hpp"
#include "private/int24.hpp"
#include "private/File.hpp"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace wav
{

class Reader
{

public:

    // May throw wav::ReadError.
    explicit Reader (std::filesystem::path const & filePath, uint32_t suggestedBufferSizeKB = 32);

    // READ ///////////////////////////////////////////////////
    //
    // Read in interleaved samples.
    // Return the number of samples read.
    // May throw wav::ReadError.
    //
    template <FloatOrDouble T>
    size_t read (T * destination, size_t samplesRequested);
    //
    template <FloatOrDouble T>
    size_t read (std::vector<T> & destination, size_t samplesRequested);
    //
    template <FloatOrDouble T>
    size_t read (std::vector<T> & destination);

    // READFRAMES /////////////////////////////////////////////
    //
    // INTERLEAVED
    //
    // Read in a number of frames, samples interleaved.
    // Return the number of frames read.
    // May throw wav::ReadError.
    //
    template <FloatOrDouble T>
    size_t readFrames (T * destination, size_t framesRequested);
    //
    template <FloatOrDouble T>
    size_t readFrames (std::vector<T> & destination, size_t framesRequested);
    //
    template <FloatOrDouble T>
    size_t readFrames (std::vector<T> & destination);
    //
    // CHANNEL-SEPARATED READING //////////////////////////////
    //
    // Read in a number of frames, grouping samples by channel.
    // Return the number of frames read.
    // May throw wav::ReadError.
    //
    template <FloatOrDouble T>
    size_t readFrames (T ** const destinations, size_t numChannels, size_t framesRequested);

    // FILE INFO //////////////////////////////////////////////

    FileInfo const & getInfo() const {return info;}
    uint16_t channels() const {return info.channels;}
    uint32_t frames() const {return info.frames;}
    uint32_t samples() const {return channels() * frames();}
    uint32_t channelMask() const {return info.channelMask;}
    uint32_t sampleRate() const {return info.sampleRate;}
    uint16_t bitDepth() const {return info.bitDepth;}
    uint16_t containerSizeInBits() const {return info.containerSizeInBits;}
    bool isLinearPCM() const {return info.isLinearPCM;}
    bool isFloatPCM() const {return info.isFloatPCM;}

private:

    File file;
    FileInfo info;

    DataChunk dataChunk;
    uint32_t dataPosition {0};    // current byte position in data chunk

    // Internal buffers for different encodings
    std::vector<uint8_t> bufferLPCMU8;
    std::vector<int16_t> bufferLPCM16;
    std::vector<int24>   bufferLPCM24;
    std::vector<int32_t> bufferLPCM32;
    std::vector<float>   bufferFloatPCM32;
    std::vector<double>  bufferFloatPCM64;

    void InitializeBuffer (uint32_t suggestedBufferSizeKB);

    // INTERLEAVED READING //////////////////////////////
    //
    // linear PCM
    template <FloatOrDouble T> void ReadLPCMU8 (T * destination, size_t samplesToRead);
    template <FloatOrDouble T> void ReadLPCM16 (T * destination, size_t samplesToRead);
    template <FloatOrDouble T> void ReadLPCM24 (T * destination, size_t samplesToRead);
    template <FloatOrDouble T> void ReadLPCM32 (T * destination, size_t samplesToRead);
    //
    // floating-point PCM
    template <FloatOrDouble T> void ReadFloatPCMNoConversion (T * destination, size_t samplesToRead);
    void ReadFloatToDouble (double * destination, size_t samplesToRead);
    void ReadDoubleToFloat (float * destination, size_t samplesToRead);

    // CHANNEL-SEPARATED READING ////////////////////////
    //
    // linear PCM
    template <FloatOrDouble T> void ReadLPCMU8 (T ** const destinations, size_t numChannels, size_t framesToRead);
    template <FloatOrDouble T> void ReadLPCM16 (T ** const destinations, size_t numChannels, size_t framesToRead);
    template <FloatOrDouble T> void ReadLPCM24 (T ** const destinations, size_t numChannels, size_t framesToRead);
    template <FloatOrDouble T> void ReadLPCM32 (T ** const destinations, size_t numChannels, size_t framesToRead);
    // floating-point PCM
    template <FloatOrDouble T> void ReadFloatPCM32 (T ** const destinations, size_t numChannels, size_t framesToRead);
    template <FloatOrDouble T> void ReadFloatPCM64 (T ** const destinations, size_t numChannels, size_t framesToRead);
};

} // namespace wav

#include "private/ReaderImplementation.hpp"

#endif
