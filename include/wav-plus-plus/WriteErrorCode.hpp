// WAV++
// Copyright (c) 2025 David Adams
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
    WriteWouldExceedWAVSizeLimit,
    InternalError
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
    FileWriteFailed,
    FramesToWriteExceedsSourceSize,
    SampleRateZero,
    SamplesToWriteExceedsSourceSize,
    SourceFrameSizeIsFractional,
    WriteWouldExceedWAVSizeLimit,

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
        case WriteErrorCode::ChannelsExceedsMax:              return WriteErrorCategory::InvalidArgument;
        case WriteErrorCode::ChannelsZero:                    return WriteErrorCategory::InvalidArgument;
        case WriteErrorCode::FramesToWriteExceedsSourceSize:  return WriteErrorCategory::InvalidArgument;
        case WriteErrorCode::SampleRateZero:                  return WriteErrorCategory::InvalidArgument;
        case WriteErrorCode::SamplesToWriteExceedsSourceSize: return WriteErrorCategory::InvalidArgument;
        case WriteErrorCode::SourceFrameSizeIsFractional:     return WriteErrorCategory::InvalidArgument;

        case WriteErrorCode::FileCloseFailed: return WriteErrorCategory::IOFailure;
        case WriteErrorCode::FileFlushFailed: return WriteErrorCategory::IOFailure;
        case WriteErrorCode::FileSeekFailed:  return WriteErrorCategory::IOFailure;
        case WriteErrorCode::FileTellFailed:  return WriteErrorCategory::IOFailure;
        case WriteErrorCode::FileWriteFailed: return WriteErrorCategory::IOFailure;

        case WriteErrorCode::WriteWouldExceedWAVSizeLimit: return WriteErrorCategory::WriteWouldExceedWAVSizeLimit;

        case WriteErrorCode::InternalError: return WriteErrorCategory::InternalError;
    }
}

} // namespace wav

#endif

