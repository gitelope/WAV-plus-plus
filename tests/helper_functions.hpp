
#ifndef wavplusplus_tests_helper_functions_hpp
#define wavplusplus_tests_helper_functions_hpp

#include "../include/wav-plus-plus/Formats.hpp"

#include <cassert>
#include <string>
#include <vector>

template <typename T>
bool areEqual (std::vector<T> const & a, std::vector<T> const & b)
{
    assert (a.size() == b.size());
    for (size_t i = 0; i < a.size(); ++i)
    {
        if (a[i] != b[i]) return false;
    }
    return true;
}

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
