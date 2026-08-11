// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_helper_functions_hpp
#define wavplusplus_tests_helper_functions_hpp

#include "../include/wav-plus-plus/Formats.hpp"

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

template <typename T>
bool areEqual (std::vector<T> const & a, std::vector<T> const & b)
{
    return a.size() == b.size() && std::equal (a.begin(), a.end(), b.begin());
}

template <typename T>
bool areEqual (std::vector<T> const & a, std::vector<T> const & b, std::size_t size)
{
    return std::equal (a.begin(), a.begin() + static_cast<std::ptrdiff_t> (size), b.begin());
}

// If expression passed in evaluates to false, add failure string to failures string.
inline void Compare (bool passes, std::string const & failure, std::string & failures)
{
    if (!passes)
    {
        if (!failures.empty()) failures += ", ";
        failures += failure;
    }
}

inline std::string exportFormatShortPrefix (wav::ExportFormat encoding)
{
    switch (encoding)
    {
        case wav::ExportFormat::LinearPCM_8:  return "u8";
        case wav::ExportFormat::LinearPCM_16: return "s16";
        case wav::ExportFormat::LinearPCM_24: return "s24";
        case wav::ExportFormat::LinearPCM_32: return "s32";
        case wav::ExportFormat::FloatPCM_32:  return "f32";
        case wav::ExportFormat::FloatPCM_64:  return "f64";
        default: return "?";
    }
}

#endif
