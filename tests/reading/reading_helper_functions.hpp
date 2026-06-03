// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_reading_helper_functions_hpp
#define wavplusplus_tests_reading_helper_functions_hpp

#include "../../include/wav-plus-plus/Formats.hpp"
#include "../../include/wav-plus-plus/private/File.hpp"

#include <bit>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

// Write byte vector to a file.
inline void writeFile (std::string const & filepath, std::vector<uint8_t> const & bytes)
{
    auto out = wav::fileForWriting (filepath);

    if (std::fwrite (bytes.data(), 1, bytes.size(), out.get()) != bytes.size())
    {
        throw std::runtime_error ("Failed to write file: " + filepath);
    }
}

// Append a little-endian uint16 to byte vector.
inline void push_uint16_le (std::vector<uint8_t> & v, uint16_t value)
{
    v.push_back (static_cast<uint8_t> (value     ));
    v.push_back (static_cast<uint8_t> (value >> 8));
}

// Append a little-endian uint32 to byte vector.
inline void push_uint32_le (std::vector<uint8_t> & v, uint32_t value)
{
    v.push_back (static_cast<uint8_t> (value      ));
    v.push_back (static_cast<uint8_t> (value >> 8 ));
    v.push_back (static_cast<uint8_t> (value >> 16));
    v.push_back (static_cast<uint8_t> (value >> 24));
}

// Append a 32-bit float to a byte vector.
inline void push_float32_le (std::vector<uint8_t> & v, float value)
{
    static_assert (std::numeric_limits<float>::is_iec559);
    static_assert (sizeof (float) == 4);
    static_assert (CHAR_BIT == 8);
    push_uint32_le (v, std::bit_cast<uint32_t> (value));
}

// Append a 4-character string to a byte vector.
inline void push_fourCC (std::vector<uint8_t> & v, const char * fourCC)
{
    v.push_back (static_cast<uint8_t> (fourCC[0]));
    v.push_back (static_cast<uint8_t> (fourCC[1]));
    v.push_back (static_cast<uint8_t> (fourCC[2]));
    v.push_back (static_cast<uint8_t> (fourCC[3]));
}

inline void push_GUID (std::vector<uint8_t> & v, wav::GUID const & guid)
{
    // GUID layout:
    // data1 (uint32)
    // data2 (uint16)
    // data3 (uint16)
    // data4 (8 byte array)
    push_uint32_le (v, guid.data1);
    push_uint16_le (v, guid.data2);
    push_uint16_le (v, guid.data3);
    for (uint8_t byte : guid.data4)
    {
        v.push_back (byte);
    }
}

#endif

