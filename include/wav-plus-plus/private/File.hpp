// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_File_hpp
#define wavplusplus_File_hpp

#include "../Exceptions.hpp"

#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>

// RAII for raw std::FILE pointers:
// - wav::File, an alias for std::unique_ptr (with a custom deleter),
// - FileCloser, the custom deleter,
// - fileForReading and fileForWriting, helper functions for opening files.

namespace wav
{

// Custom deleter for std::unique_ptr<std::FILE>.
// Substitutes std::fclose for the default deleter, delete, whose use
// on a FILE * from std::fopen would result in undefined behavior.
//
// std::unique_ptr<std::FILE, decltype(&std::fclose)> causes GCC to emit
// a warning when compiling with -Wignored-attributes so we use a lambda instead.
//
// Discards fclose's return value rather than checking it and throwing,
// because unique_ptr's destructor may run during stack unwinding where
// any throw calls std::terminate. Use file.release() and std::fclose manually
// when you need to detect close errors (see Writer::Close).
inline constexpr auto FileCloser = [](std::FILE * f) noexcept { std::fclose(f); };

using File = std::unique_ptr<std::FILE, decltype(FileCloser)>;

inline File fileForReading (std::string const & filepath)
{
    if (! std::filesystem::is_regular_file (filepath))
    {
        throw ReadError ("File does not exist: " + filepath);
    }
    File f (std::fopen (filepath.c_str(), "rb"));
    if (! f)
        throw ReadError ("Failed to open file for reading: " + filepath);
    return f;
}

inline File fileForWriting (std::string const & filepath)
{
    File f (std::fopen (filepath.c_str(), "wb"));
    if (! f)
        throw WriteError ("Failed to open file for writing: " + filepath);
    return f;
}

}

#endif

