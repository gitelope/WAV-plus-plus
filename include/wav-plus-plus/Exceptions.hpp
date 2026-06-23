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
    MigrationCategory,
    FileOpenFailure,
    IOFailure,
    InvalidFile,
    UnsupportedFormat,
    InvalidArgument,
    InternalError
};

enum class ReadErrorCode
{
    MigrationCode,
    ChannelsZero,
};

constexpr ReadErrorCategory categoryFor (ReadErrorCode code)
{
    switch (code)
    {
        case ReadErrorCode::ChannelsZero: return ReadErrorCategory::InvalidFile; break;
        default: return ReadErrorCategory::MigrationCategory;
    }
}

class ReadError : public std::runtime_error
{

public:

    explicit ReadError (ReadErrorCode code, std::string const & message)
    :std::runtime_error (message), code_ (code)
    {}

    explicit ReadError (std::string const & message)
    :std::runtime_error (message), code_ (ReadErrorCode::MigrationCode)
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
