// WAV++ (c) 2025 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_ReadErrorCode_hpp
#define wavplusplus_ReadErrorCode_hpp

namespace wav
{

enum class ReadErrorCategory
{
    FileOpenFailure,
    IOFailure,
    InvalidFile,
    UnsupportedFormat,
    InvalidArgument,
    InternalError
};

enum class ReadErrorCode
{
    BitsPerSampleChannelsBlockAlignMismatch,
    BlockAlignZero,
    BlockAlignNotDivisibleByChannels,
    ChannelsZero,
    ChannelsExceedsMax,
    ChannelsRequestedExceedsChannelCount,
    DataChunkBeforeFmtChunk,
    DataChunkSizeNotDivisibleByBlockAlign,
    DeclaredChunkEndExceedsRiffEnd,
    DestinationFrameSizeIsFractional,
    ExtensibleFmtSizeCodeMismatch,
    ExtensibleSubformatNotSupported,
    FactChunkSizeNotFour,
    FailedToOpenFileForReading,
    FileReadFailed,
    FileSeekFailed,
    FileTellFailed,
    FramesRequestedExceedsDestinationSize,
    IncompleteChunkHeader,
    IncompleteRiffHeader,
    InvalidCbSize,
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
    SampleRateZero,
    SamplesRequestedExceedsDestinationSize,
    UnsupportedFloatContainerSize,
    UnsupportedFmtChunkSize,
    UnsupportedFormatCode,
    UnsupportedLPCMContainerSize,
    ValidBitsPerSampleExceedsLinearContainerSize,
    ValidBitsPerSampleNotEqualToFloatContainerSize,
    ValidBitsPerSampleZero,

    // InternalError should never be thrown if program is working correctly.
    // Kind of like an assert for release builds. Can be caught and logged
    // instead of crashing.
    InternalError
};

constexpr ReadErrorCategory categoryFor (ReadErrorCode code)
{
    switch (code)
    {
        case ReadErrorCode::FailedToOpenFileForReading: return ReadErrorCategory::FileOpenFailure;

        case ReadErrorCode::FileSeekFailed: return ReadErrorCategory::IOFailure;
        case ReadErrorCode::FileTellFailed: return ReadErrorCategory::IOFailure;
        case ReadErrorCode::FileReadFailed: return ReadErrorCategory::IOFailure;

        case ReadErrorCode::BitsPerSampleChannelsBlockAlignMismatch:        return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::BlockAlignZero:                                 return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::BlockAlignNotDivisibleByChannels:               return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::ChannelsZero:                                   return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::DataChunkBeforeFmtChunk:                        return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::DataChunkSizeNotDivisibleByBlockAlign:          return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::DeclaredChunkEndExceedsRiffEnd:                 return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::ExtensibleFmtSizeCodeMismatch:                  return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::FactChunkSizeNotFour:                           return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::IncompleteChunkHeader:                          return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::IncompleteRiffHeader:                           return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::InvalidCbSize:                                  return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::MissingDataChunk:                               return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::MissingFmtChunk:                                return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::MultipleDataChunks:                             return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::MultipleFactChunks:                             return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::MultipleFmtChunks:                              return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::MultiplePeakChunks:                             return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::MultipleRiffChunks:                             return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::PeakChunkBeforeFmtChunk:                        return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::PeakChunkSizeMismatchesChannels:                return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::PeakChunkSizeTooSmallForVersionField:           return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::ReadPastEndOfVector:                            return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::RiffEndExceedsFileSize:                         return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::RiffIdNotFound:                                 return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::RiffSubformatNotWave:                           return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::SampleRateZero:                                 return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::ValidBitsPerSampleZero:                         return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::ValidBitsPerSampleExceedsLinearContainerSize:   return ReadErrorCategory::InvalidFile;
        case ReadErrorCode::ValidBitsPerSampleNotEqualToFloatContainerSize: return ReadErrorCategory::InvalidFile;

        case ReadErrorCode::ChannelsRequestedExceedsChannelCount:   return ReadErrorCategory::InvalidArgument;
        case ReadErrorCode::DestinationFrameSizeIsFractional:       return ReadErrorCategory::InvalidArgument;
        case ReadErrorCode::FramesRequestedExceedsDestinationSize:  return ReadErrorCategory::InvalidArgument;
        case ReadErrorCode::SamplesRequestedExceedsDestinationSize: return ReadErrorCategory::InvalidArgument;

        case ReadErrorCode::ChannelsExceedsMax:                return ReadErrorCategory::UnsupportedFormat;
        case ReadErrorCode::ExtensibleSubformatNotSupported:  return ReadErrorCategory::UnsupportedFormat;
        case ReadErrorCode::UnsupportedFloatContainerSize:    return ReadErrorCategory::UnsupportedFormat;
        case ReadErrorCode::UnsupportedFmtChunkSize:          return ReadErrorCategory::UnsupportedFormat;
        case ReadErrorCode::UnsupportedFormatCode:            return ReadErrorCategory::UnsupportedFormat;
        case ReadErrorCode::UnsupportedLPCMContainerSize:     return ReadErrorCategory::UnsupportedFormat;

        case ReadErrorCode::InternalError: return ReadErrorCategory::InternalError;
    }
}

} // namespace wav

#endif

