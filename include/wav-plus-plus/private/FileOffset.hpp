// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_FileOffset_hpp
#define wavplusplus_FileOffset_hpp

#include "../Exceptions.hpp"

#include <cstdint>
#include <cstdio>

namespace wav
{

// Returns 0 on success, non-zero on failure.
[[nodiscard]] inline int seek (std::FILE * f, std::int64_t offset, int origin) noexcept
{
#if defined(_WIN32)
    return ::_fseeki64 (f, offset, origin);
#else
    static_assert (sizeof (long) >= 8, "64-bit long required for std::fseek on POSIX.");
    return std::fseek (f, static_cast<long> (offset), origin);
#endif
}

// Returns current position, -1 on failure.
[[nodiscard]] inline std::int64_t tell (std::FILE * f) noexcept
{
#if defined(_WIN32)
    return ::_ftelli64 (f);
#else
    static_assert (sizeof (long) >= 8, "64-bit long required for std::ftell on POSIX.");
    return std::ftell (f);
#endif
}

// Returns file size in bytes.
//
// Position-preserving on success.
// Throws wav::ReadError on failure.
inline std::int64_t fileSize (std::FILE * f)
{
    const auto currentPosition = tell (f);
    if (currentPosition == -1)
        throw ReadError ("Failed to get current file position.");

    if (seek (f, 0, SEEK_END) != 0)
        throw ReadError ("Failed to seek to end of file; file position is undefined.");

    const auto size = tell (f);
    if (size == -1)
        throw ReadError ("Failed to get file size; file position is at end-of-file.");

    if (seek (f, currentPosition, SEEK_SET) != 0)
        throw ReadError ("Failed to restore file position; file position is undefined.");

    return size;
}

} // namespace wav

#endif
