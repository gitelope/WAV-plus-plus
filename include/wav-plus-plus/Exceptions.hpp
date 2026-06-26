// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_Exceptions_hpp
#define wavplusplus_Exceptions_hpp

#include <stdexcept>
#include <string>

namespace wav
{

///////////////
// ReadError //
///////////////

enum class ReadErrorCategory
{
    FileOpenFailure,
    IOFailure,
    InvalidFile,
    UnsupportedFormat,
    InvalidArgument,
    InternalError,

    MigrationCategoryToBeRemoved,
};

enum class ReadErrorCode
{
    BlockAlignNotDivisibleByChannels,
    BlockAlignZero,
    // BitsPerSampleExceedsSupportedMax,
    // BitsPerSampleNotMultipleOf8,
    // BitsPerSampleZero,
    ChannelsExceedMax,
    ChannelsZero,
    ExtensibleBitsPerSampleChannelsBlockAlignMismatch,
    ExtensibleFmtSizeCodeMismatch,
    ExtensibleSubformatNotSupported,
    FloatBitsPerSampleChannelsBlockAlignMismatch,
    InvalidCbSize,
    MultipleFmtChunks,
    SampleRateZero,
    UnsupportedFloatContainerSize,
    UnsupportedFmtChunkSize,
    UnsupportedFormatCode,
    UnsupportedLPCMContainerSize,

    // InternalError should never be thrown if program is working correctly.
    // Kind of like an assert for release builds. Can be caught and logged
    // instead of crashing.
    InternalError,

    MigrationCodeToBeRemoved,
};

constexpr ReadErrorCategory categoryFor (ReadErrorCode code)
{
    switch (code)
    {
        // case ReadErrorCode::BitsPerSampleZero:                                 return ReadErrorCategory::InvalidFile;
        // case ReadErrorCode::BitsPerSampleNotMultipleOf8:                       return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::BlockAlignNotDivisibleByChannels:                  return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::BlockAlignZero:                                    return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::ChannelsZero:                                      return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::ExtensibleBitsPerSampleChannelsBlockAlignMismatch: return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::ExtensibleFmtSizeCodeMismatch:                     return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::FloatBitsPerSampleChannelsBlockAlignMismatch:      return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::InvalidCbSize:                                     return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::MultipleFmtChunks:                                 return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::SampleRateZero:                                    return ReadErrorCategory::InvalidFile;

        // case ReadErrorCode::BitsPerSampleExceedsSupportedMax: return ReadErrorCategory::UnsupportedFormat;
        case ReadErrorCode::ChannelsExceedMax:                return ReadErrorCategory::UnsupportedFormat;
        case ReadErrorCode::ExtensibleSubformatNotSupported:  return ReadErrorCategory::UnsupportedFormat;
        case ReadErrorCode::UnsupportedFloatContainerSize:    return ReadErrorCategory::UnsupportedFormat;
        case ReadErrorCode::UnsupportedFmtChunkSize:          return ReadErrorCategory::UnsupportedFormat;
        case ReadErrorCode::UnsupportedFormatCode:            return ReadErrorCategory::UnsupportedFormat;
        case ReadErrorCode::UnsupportedLPCMContainerSize:     return ReadErrorCategory::UnsupportedFormat;

        case ReadErrorCode::InternalError: return ReadErrorCategory::InternalError;

        default: return ReadErrorCategory::MigrationCategoryToBeRemoved;
    }
}

class ReadError : public std::runtime_error
{

public:

    explicit ReadError (ReadErrorCode code, std::string const & message)
    :std::runtime_error (message), code_ (code)
    {}

    explicit ReadError (std::string const & message)
    :std::runtime_error (message), code_ (ReadErrorCode::MigrationCodeToBeRemoved)
    {}

    ReadErrorCategory category() const {return categoryFor (code_);}
    ReadErrorCode code() const {return code_;}

    ReadErrorCode code_;
};

////////////////
// WriteError //
////////////////

class WriteError : public std::runtime_error
{
public:
    explicit WriteError (std::string const & message)
    :std::runtime_error (message)
    {}
};

}
#endif
