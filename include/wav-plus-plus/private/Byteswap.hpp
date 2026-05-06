// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_Byteswap_hpp
#define wavplusplus_Byteswap_hpp

// When migrating to C++23 or higher, wav::byteswap can make use of
// std::byteswap to simplify (and possibly achieve higher performance).
// It would still be useful to retain wav::byteswap b/c it offers
// convenience for float and double by handling the std::bit_cast to
// uint32_t and uint64_t before the byte swap and also the std::bit_cast
// back to the original type afterwards.

#include <bit>
#include <cstdint>
#include <type_traits>

namespace wav
{

template <typename T>
constexpr T byteswap (T value) noexcept
{
    static_assert(
                  std::is_same_v<T, std::int16_t> ||
                  std::is_same_v<T, std::int32_t> ||
                  std::is_same_v<T, std::int64_t> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double>,
                  "wav::byteswap currently supports int16_t, int32_t, int64_t, float, and double only."
                  );

    if constexpr (sizeof (T) == 2)
    {
        auto u = std::bit_cast<std::uint16_t> (value);
        u = static_cast<std::uint16_t> ((u << 8) | (u >> 8));
        return std::bit_cast<T> (u);
    }
    else if constexpr (sizeof (T) == 4)
    {
        auto u = std::bit_cast<std::uint32_t> (value);
        u = (u << 24) |
        ((u << 8)  & 0x00FF0000u) |
        ((u >> 8)  & 0x0000FF00u) |
        (u >> 24);
        return std::bit_cast<T> (u);
    }
    else // sizeof (T) == 8
    {
        auto u = std::bit_cast<std::uint64_t> (value);
        u = (u << 56) |
        ((u << 40) & 0x00FF000000000000ull) |
        ((u << 24) & 0x0000FF0000000000ull) |
        ((u << 8)  & 0x000000FF00000000ull) |
        ((u >> 8)  & 0x00000000FF000000ull) |
        ((u >> 24) & 0x0000000000FF0000ull) |
        ((u >> 40) & 0x000000000000FF00ull) |
        (u >> 56);
        return std::bit_cast<T> (u);
    }
}

} // namespace wav

#endif
