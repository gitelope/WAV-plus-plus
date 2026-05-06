// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_Byteswap_hpp
#define wavplusplus_tests_Byteswap_hpp

#include "../../include/wav-plus-plus/private/Byteswap.hpp"

#include <bit>
#include <cstdint>
#include <string>

template <typename T>
void Test (T input, T expected, std::string const & type, TestLog & log)
{
    // test that output is expected and that round trip produces the original input
    const T actual {wav::byteswap (input)};
    const T roundTrip {wav::byteswap (actual)};
    log.Record (actual == expected && roundTrip == input, "Utilities - byteswap - " + type);
}

void TestByteswap (TestLog & log)
{
    Test (std::bit_cast<int16_t> (uint16_t {0x0123}),
          std::bit_cast<int16_t> (uint16_t {0x2301}),             "int16_t", log);

    Test (std::bit_cast<int32_t> (uint32_t {0x01234567}),
          std::bit_cast<int32_t> (uint32_t {0x67452301}),         "int32_t", log);

    Test (std::bit_cast<int64_t> (uint64_t {0x0123456789ABCDEF}),
          std::bit_cast<int64_t> (uint64_t {0xEFCDAB8967452301}), "int64_t", log);

    Test (std::bit_cast<float> (uint32_t {0x01234567}),
          std::bit_cast<float> (uint32_t {0x67452301}),           "float",   log);

    Test (std::bit_cast<double> (uint64_t {0x0123456789ABCDEF}),
          std::bit_cast<double> (uint64_t {0xEFCDAB8967452301}),  "double",  log);
}

#endif
