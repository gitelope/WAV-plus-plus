// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_Exceptions_hpp
#define wavplusplus_Exceptions_hpp

#include <stdexcept>
#include <string>

#include "ReadErrorCode.hpp"

namespace wav
{

class ReadError : public std::runtime_error
{

public:

    explicit ReadError (ReadErrorCode code, std::string const & message)
    :std::runtime_error (message), code_ (code)
    {}

    ReadErrorCategory category() const {return categoryFor (code_);}
    ReadErrorCode code() const {return code_;}

private:
    ReadErrorCode code_;
};

class WriteError : public std::runtime_error
{
public:
    explicit WriteError (std::string const & message)
    :std::runtime_error (message)
    {}
};

} // namespace wav

#endif
