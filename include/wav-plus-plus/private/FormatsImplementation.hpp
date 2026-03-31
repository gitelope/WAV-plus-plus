
#ifndef FormatsImplementation_hpp
#define FormatsImplementation_hpp

#include "../Exceptions.hpp"

#include <format>
#include <sstream>

namespace wav
{

constexpr bool isValidFormatCode (uint16_t formatCode)
{
    return
    formatCode == FormatCode::LinearPCM||
    formatCode == FormatCode::FloatPCM ||
    formatCode == FormatCode::Extensible;
}

constexpr std::pair <uint16_t, uint16_t> bitDepthAndFormatCode (ExportFormat ef) noexcept
{
    switch (ef)
    {
        case ExportFormat::LinearPCM_8:  return {8,  FormatCode::LinearPCM};
        case ExportFormat::LinearPCM_16: return {16, FormatCode::LinearPCM};
        case ExportFormat::LinearPCM_24: return {24, FormatCode::LinearPCM};
        case ExportFormat::LinearPCM_32: return {32, FormatCode::LinearPCM};
        case ExportFormat::FloatPCM_32:  return {32, FormatCode::FloatPCM};
        case ExportFormat::FloatPCM_64:  return {64, FormatCode::FloatPCM};
    }
}

constexpr bool isValidFormatChunkSize (uint32_t chunkSize)
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

inline uint32_t channelMaskFromPositions (std::vector<SpeakerPosition> const & positions)
{
    uint32_t channelMask {0};
    for (auto position : positions)
    {
        const auto positionBit {static_cast<uint32_t> (position)};
        if (channelMask & positionBit)
        {
            throw WriteError ("Duplicate speaker position in channel mask.");
        }
        channelMask |= positionBit;
    }
    return channelMask;
}

inline std::vector<SpeakerPosition> speakerPositionsFromMask (uint32_t channelMask)
{
    constexpr std::array<SpeakerPosition, 18> allPositions
    {
        SpeakerPosition::FrontLeft,
        SpeakerPosition::FrontRight,
        SpeakerPosition::FrontCenter,
        SpeakerPosition::LowFrequency,
        SpeakerPosition::BackLeft,
        SpeakerPosition::BackRight,
        SpeakerPosition::FrontLeftOfCenter,
        SpeakerPosition::FrontRightOfCenter,
        SpeakerPosition::BackCenter,
        SpeakerPosition::SideLeft,
        SpeakerPosition::SideRight,
        SpeakerPosition::TopCenter,
        SpeakerPosition::TopFrontLeft,
        SpeakerPosition::TopFrontCenter,
        SpeakerPosition::TopFrontRight,
        SpeakerPosition::TopBackLeft,
        SpeakerPosition::TopBackCenter,
        SpeakerPosition::TopBackRight
    };

    std::vector<SpeakerPosition> result;
    for (auto position : allPositions)
    {
        const auto positionBit {static_cast<uint32_t> (position)};
        if (channelMask & positionBit)
        {
            result.push_back (position);
        }
    }
    return result;
}

}
// namespace wav

#endif
