
#ifndef wavplusplus_DequantizationConstants_hpp
#define wavplusplus_DequantizationConstants_hpp

namespace wav
{

// The maxes are the maximum absolute value e.g the minimum uint8
// possible is -128 and the maximum is 127 and uint8Max = abs(-128)

inline constexpr double uint8Offset {128.};        // 2^7
inline constexpr double uint8MaxAbs {128.};        // 2^7
inline constexpr double int16MaxAbs {32768.};      // 2^15
inline constexpr double int24MaxAbs {8388608.};    // 2^23
inline constexpr double int32MaxAbs {2147483648.}; // 2^31

} // namespace wav

#endif
