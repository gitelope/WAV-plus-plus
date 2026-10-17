// WAV++
// Copyright (c) 2025 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_QuantDequantConstants_hpp
#define wavplusplus_QuantDequantConstants_hpp

namespace wav
{

namespace quantization
{

inline constexpr double uint8Scale  {127}; // 2^7 - 1
inline constexpr double uint8Offset {128}; // 2^7
inline constexpr double uint8Min    {  1};
inline constexpr double uint8Max    {255}; // 2^8 - 1

inline constexpr double int16Scale { 32767}; //   2^15 - 1
inline constexpr double int16Min   {-32767}; // -(2^15 - 1)
inline constexpr double int16Max   { 32767}; //   2^15 - 1

inline constexpr double int24Scale { 8388607}; //   2^23 - 1
inline constexpr double int24Min   {-8388607}; // -(2^23 - 1)
inline constexpr double int24Max   { 8388607}; //   2^23 - 1

inline constexpr double int32Scale { 2147483647}; //   2^31 - 1
inline constexpr double int32Min   {-2147483647}; // -(2^31 - 1)
inline constexpr double int32Max   { 2147483647}; //   2^31 - 1

}

namespace dequantization
{

inline constexpr double uint8Scale  {128}; // 2^7
static constexpr double uint8Offset {128};

inline constexpr double int16Scale  {32768}; // 2^15

inline constexpr double int24Scale  {8388608}; // 2^23

inline constexpr double int32Scale  {2147483648}; // 2^31

}

}

#endif
