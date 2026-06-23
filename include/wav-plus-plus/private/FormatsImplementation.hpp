// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_FormatsImplementation_hpp
#define wavplusplus_FormatsImplementation_hpp

#include "../Exceptions.hpp"

#include <format>
#include <sstream>

namespace wav
{

constexpr bool isSupportedFormatCode (uint16_t formatCode)
{
    return
    formatCode == FormatCode::LinearPCM||
    formatCode == FormatCode::FloatPCM ||
    formatCode == FormatCode::Extensible;
}

constexpr std::pair <uint16_t, uint16_t> bitDepthAndFormatCode (ExportFormat ef)
{
    switch (ef)
    {
        case ExportFormat::LinearPCM_8:  return {static_cast<uint16_t> (8),  FormatCode::LinearPCM};
        case ExportFormat::LinearPCM_16: return {static_cast<uint16_t> (16), FormatCode::LinearPCM};
        case ExportFormat::LinearPCM_24: return {static_cast<uint16_t> (24), FormatCode::LinearPCM};
        case ExportFormat::LinearPCM_32: return {static_cast<uint16_t> (32), FormatCode::LinearPCM};
        case ExportFormat::FloatPCM_32:  return {static_cast<uint16_t> (32), FormatCode::FloatPCM};
        case ExportFormat::FloatPCM_64:  return {static_cast<uint16_t> (64), FormatCode::FloatPCM};
        default: throw wav::WriteError ("Unsupported ExportFormat.");
    }
}

constexpr bool isSupportedFormatChunkSize (uint32_t chunkSize)
{
    return
    chunkSize == FormatChunkSize::WAVEFORMAT         ||
    chunkSize == FormatChunkSize::PCMWAVEFORMAT      ||
    chunkSize == FormatChunkSize::WAVEFORMATEX       ||
    chunkSize == FormatChunkSize::WAVEFORMATEXTENSIBLE;
}

inline std::ostream & operator<< (std::ostream & os, GUID const & guid)
{
    os << std::format ("{:08X}-{:04X}-{:04X}-{:02X}{:02X}-{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}",
                       guid.data1,
                       guid.data2,
                       guid.data3,
                       guid.data4[0], guid.data4[1],
                       guid.data4[2], guid.data4[3], guid.data4[4], guid.data4[5], guid.data4[6], guid.data4[7]);
    return os;
}

inline std::string GUID::str() const
{
    std::ostringstream oss;
    oss << *this;
    return oss.str();
}

}
// namespace wav

#endif
