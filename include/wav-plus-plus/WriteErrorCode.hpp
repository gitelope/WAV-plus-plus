// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_WriteErrorCode_hpp
#define wavplusplus_WriteErrorCode_hpp

namespace wav
{

enum class WriteErrorCategory
{
    FileOpenFailure,
    IOFailure,
    InvalidArgument,
    UnsupportedFormat,
    WouldExceedDataLimit,
    InternalError,

    MigrationCategoryToBeRemoved
};

enum class WriteErrorCode
{
    BufferSizeZero,
    ChannelMaskInvalid,
    ChannelsExceedsMax,
    ChannelsZero,
    FailedToOpenFileForWriting,
    FileCloseFailed,
    FileFlushFailed,
    FileSeekFailed,
    FileTellFailed,
    FramesToWriteExceedsSourceSize,
    SampleRateZero,
    SamplesToWriteExceedsSourceSize,
    SourceFrameSizeIsFractional,

    MigrationCodeToBeRemoved,

    // InternalError should never be thrown if program is working correctly.
    // Kind of like an assert for release builds. Can be caught and logged
    // instead of crashing.
    InternalError
};

constexpr WriteErrorCategory categoryFor (WriteErrorCode code)
{
    switch (code)
    {
        case WriteErrorCode::FailedToOpenFileForWriting: return WriteErrorCategory::FileOpenFailure;

        case WriteErrorCode::BufferSizeZero:                  return WriteErrorCategory::InvalidArgument;
        case WriteErrorCode::ChannelMaskInvalid:              return WriteErrorCategory::InvalidArgument;
        case WriteErrorCode::ChannelsZero:                    return WriteErrorCategory::InvalidArgument;
        case WriteErrorCode::FramesToWriteExceedsSourceSize:  return WriteErrorCategory::InvalidArgument;
        case WriteErrorCode::SampleRateZero:                  return WriteErrorCategory::InvalidArgument;
        case WriteErrorCode::SamplesToWriteExceedsSourceSize: return WriteErrorCategory::InvalidArgument;
        case WriteErrorCode::SourceFrameSizeIsFractional:     return WriteErrorCategory::InvalidArgument;

        case WriteErrorCode::FileCloseFailed: return WriteErrorCategory::IOFailure;
        case WriteErrorCode::FileFlushFailed: return WriteErrorCategory::IOFailure;
        case WriteErrorCode::FileSeekFailed:  return WriteErrorCategory::IOFailure;
        case WriteErrorCode::FileTellFailed:  return WriteErrorCategory::IOFailure;

        case WriteErrorCode::ChannelsExceedsMax: return WriteErrorCategory::UnsupportedFormat;

        case WriteErrorCode::MigrationCodeToBeRemoved: return WriteErrorCategory::MigrationCategoryToBeRemoved;

        case WriteErrorCode::InternalError: return WriteErrorCategory::InternalError;
    }
}

} // namespace wav

#endif

