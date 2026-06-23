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
    ChannelsExceedMax,
    ChannelsZero,
    MultipleFmtChunks,
    SampleRateZero,

    MigrationCodeToBeRemoved,
};

constexpr ReadErrorCategory categoryFor (ReadErrorCode code)
{
    switch (code)
    {
        case ReadErrorCode::ChannelsExceedMax:                return ReadErrorCategory::UnsupportedFormat; break;

        case ReadErrorCode::BlockAlignNotDivisibleByChannels: return ReadErrorCategory::InvalidFile; break;
        case ReadErrorCode::BlockAlignZero:                   return ReadErrorCategory::InvalidFile; break;
        case ReadErrorCode::ChannelsZero:                     return ReadErrorCategory::InvalidFile; break;
        case ReadErrorCode::MultipleFmtChunks:                return ReadErrorCategory::InvalidFile; break;
        case ReadErrorCode::SampleRateZero:                   return ReadErrorCategory::InvalidFile; break;

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
