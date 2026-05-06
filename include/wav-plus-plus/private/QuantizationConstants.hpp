// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_QuantizationConstants_hpp
#define wavplusplus_QuantizationConstants_hpp

namespace wav
{

inline constexpr double uint8Scale              {127}; // 2^7 - 1
inline constexpr double uint8QuantizationOffset {128}; // 2^7
inline constexpr double uint8Min                {  1};
inline constexpr double uint8Max                {255}; // 2^8 - 1

inline constexpr double int16Scale { 32767}; //   2^15 - 1
inline constexpr double int16Min   {-32767}; // -(2^15 - 1)
inline constexpr double int16Max   { 32767}; //   2^15 - 1

inline constexpr double int24Scale { 8388607}; //   2^23 - 1
inline constexpr double int24Min   {-8388607}; // -(2^23 - 1)
inline constexpr double int24Max   { 8388607}; //   2^23 - 1

inline constexpr double int32Scale { 2147483647}; //   2^31 - 1
inline constexpr double int32Min   {-2147483647}; // -(2^31 - 1)
inline constexpr double int32Max   { 2147483647}; //   2^31 - 1

} // namespace wav

#endif
