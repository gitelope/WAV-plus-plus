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
    BlockAlignEqualsZero,
    BlockAlignNotDivisibleByChannels,
    BitDepthEqualsZero,
    ChannelsExceedMax,
    ChannelsEqualsZero,
    ChannelsRequestedExceedsChannelCount,
    DataChunkBeforeFmtChunk,
    DataChunkSizeNotDivisibleByBlockAlign,
    DeclaredChunkEndExceedsRiffEnd,
    DestinationFrameSizeIsFractional,
    ExtensibleBitsPerSampleChannelsBlockAlignMismatch,
    ExtensibleFmtSizeCodeMismatch,
    ExtensibleSubformatNotSupported,
    FactChunkSizeNotFour,
    FailedToOpenFileForReading,
    FileReadFailed,
    FileSeekFailed,
    FileTellFailed,
    FloatBitDepthNotEqualToContainerSize,
    FramesRequestedExceedsDestinationSize,
    IncompleteChunkHeader,
    IncompleteRiffHeader,
    InvalidCbSize,
    LinearBitDepthExceedsContainerSize,
    MissingDataChunk,
    MissingFmtChunk,
    MultipleDataChunks,
    MultipleFactChunks,
    MultipleFmtChunks,
    MultiplePeakChunks,
    MultipleRiffChunks,
    PeakChunkBeforeFmtChunk,
    PeakChunkSizeMismatchesChannels,
    PeakChunkSizeTooSmallForVersionField,
    ReadPastEndOfVector,
    RiffEndExceedsFileSize,
    RiffIdNotFound,
    RiffSubformatNotWave,
    SampleRateEqualsZero,
    SamplesRequestedExceedsDestinationSize,
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
        case ReadErrorCode::FailedToOpenFileForReading: return ReadErrorCategory::FileOpenFailure;

        case ReadErrorCode::FileSeekFailed: return ReadErrorCategory::IOFailure;
        case ReadErrorCode::FileTellFailed: return ReadErrorCategory::IOFailure;
        case ReadErrorCode::FileReadFailed: return ReadErrorCategory::IOFailure;

        case ReadErrorCode::BlockAlignEqualsZero:                              return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::BlockAlignNotDivisibleByChannels:                  return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::BitDepthEqualsZero:                                return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::ChannelsEqualsZero:                                return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::DataChunkBeforeFmtChunk:                           return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::DataChunkSizeNotDivisibleByBlockAlign:             return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::DeclaredChunkEndExceedsRiffEnd:                    return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::ExtensibleBitsPerSampleChannelsBlockAlignMismatch: return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::ExtensibleFmtSizeCodeMismatch:                     return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::FactChunkSizeNotFour:                              return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::FloatBitDepthNotEqualToContainerSize:              return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::IncompleteChunkHeader:                             return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::IncompleteRiffHeader:                              return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::InvalidCbSize:                                     return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::LinearBitDepthExceedsContainerSize:                return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::MissingDataChunk:                                  return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::MissingFmtChunk:                                   return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::MultipleDataChunks:                                return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::MultipleFactChunks:                                return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::MultipleFmtChunks:                                 return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::MultiplePeakChunks:                                return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::MultipleRiffChunks:                                return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::PeakChunkBeforeFmtChunk:                           return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::PeakChunkSizeMismatchesChannels:                   return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::PeakChunkSizeTooSmallForVersionField:              return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::ReadPastEndOfVector:                               return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::RiffEndExceedsFileSize:                            return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::RiffIdNotFound:                                    return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::RiffSubformatNotWave:                              return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::SampleRateEqualsZero:                              return ReadErrorCategory::InvalidFile;

        case ReadErrorCode::ChannelsRequestedExceedsChannelCount:   return ReadErrorCategory::InvalidArgument;
        case ReadErrorCode::DestinationFrameSizeIsFractional:       return ReadErrorCategory::InvalidArgument;
        case ReadErrorCode::FramesRequestedExceedsDestinationSize:  return ReadErrorCategory::InvalidArgument;
        case ReadErrorCode::SamplesRequestedExceedsDestinationSize: return ReadErrorCategory::InvalidArgument;

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
