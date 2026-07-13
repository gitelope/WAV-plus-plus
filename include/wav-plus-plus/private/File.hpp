// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_File_hpp
#define wavplusplus_File_hpp

#include "../Exceptions.hpp"

#include <cstdio>
#include <filesystem>
#include <memory>

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

inline File fileForReading (std::filesystem::path const & filepath)
{
#if defined(_WIN32)
    static_assert (std::is_same_v<std::filesystem::path::value_type, wchar_t>, "Expected wchar_t paths on Windows");
    std::FILE * raw = nullptr;
    ::_wfopen_s (&raw, filepath.c_str(), L"rb");
    File f (raw);
#else
    File f (std::fopen (filepath.c_str(), "rb"));
#endif
    if (f == nullptr) throw ReadError (ReadErrorCode::FailedToOpenFileForReading, "Failed to open file for reading.");
    return f;
}

inline File fileForWriting (std::filesystem::path const & filepath)
{
#if defined(_WIN32)
    static_assert (std::is_same_v<std::filesystem::path::value_type, wchar_t>, "Expected wchar_t paths on Windows");
    std::FILE * raw = nullptr;
    ::_wfopen_s (&raw, filepath.c_str(), L"wb");
    File f (raw);
#else
    File f (std::fopen (filepath.c_str(), "wb"));
#endif
    if (f == nullptr) throw WriteError (WriteErrorCode::FailedToOpenFileForWriting, "Failed to open file for writing.");
    return f;
}

}

#endif

