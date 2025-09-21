
#ifndef wavplusplus_int24_hpp
#define wavplusplus_int24_hpp

#include "Formats.hpp"
#include <array>
#include <cstdint>
#include <cstddef>

namespace wav
{

struct int24
{
    std::array<std::byte, 3> bytes;

    constexpr int24() noexcept
    :bytes {std::byte{0}, std::byte{0}, std::byte{0}}
    {}

    // construct from quantized value (in the form of a float or double)
    template <FloatOrDouble T>
    constexpr int24 (T quantizedValue) noexcept
    {
        const auto integer {static_cast<std::int32_t> (quantizedValue)};
        bytes[0] = static_cast<std::byte> (integer);
        bytes[1] = static_cast<std::byte> (integer >> 8);
        bytes[2] = static_cast<std::byte> (integer >> 16);
    }

    constexpr int24 (std::byte b0, std::byte b1, std::byte b2) noexcept
    {
        bytes[0] = b0;
        bytes[1] = b1;
        bytes[2] = b2;
    }

    // conversion to int32_t
    constexpr operator int32_t() const noexcept
    {
        auto result =
        static_cast<int32_t> (bytes[0])      |
        static_cast<int32_t> (bytes[1]) <<  8|
        static_cast<int32_t> (bytes[2]) << 16;

        // sign extension for 24-bit to 32-bit
        if (result & 0x800000) result |= 0xFF000000;
        return result;
    }
};

} // namespace wav

#endif

