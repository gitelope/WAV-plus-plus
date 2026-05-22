// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_FileOffset_hpp
#define wavplusplus_FileOffset_hpp

#include <cstdint>
#include <cstdio>

namespace wav
{

// Returns 0 on success, nonzero on failure. Matches std::fseek convention.
[[nodiscard]] inline int seek (std::FILE * f, std::int64_t offset, int origin) noexcept
{
#if defined(_WIN32)
    return ::_fseeki64 (f, offset, origin);
#else
    static_assert (sizeof (long) >= 8, "64-bit long required for std::fseek on POSIX.");
    return std::fseek (f, static_cast<long> (offset), origin);
#endif
}

// Returns current position, or -1 on failure. Matches std::ftell convention.
[[nodiscard]] inline std::int64_t tell (std::FILE * f) noexcept
{
#if defined(_WIN32)
    return ::_ftelli64 (f);
#else
    static_assert (sizeof (long) >= 8, "64-bit long required for std::ftell on POSIX.");
    return std::ftell (f);
#endif
}

} // namespace wav

#endif

