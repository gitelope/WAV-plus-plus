
#ifndef wavplusplus_int24_hpp
#define wavplusplus_int24_hpp

namespace wav
{

struct int24
{
    std::array<std::byte, 3> bytes;

    constexpr int24() noexcept
    :bytes {std::byte{0}, std::byte{0}, std::byte{0}}
    {}

    template <FloatOrDouble T>
    constexpr int24 (T quantizedValue) noexcept
    {
        const auto integer {static_cast<std::int32_t> (quantizedValue)};
        bytes[0] = static_cast<std::byte> (integer);
        bytes[1] = static_cast<std::byte> (integer >> 8);
        bytes[2] = static_cast<std::byte> (integer >> 16);
    }
};

} // namespace wav

#endif

