// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_Formats_hpp
#define wavplusplus_Formats_hpp

#include <array>
#include <climits>
#include <concepts>
#include <cstdint>
#include <limits>
#include <ostream>
#include <string>
#include <utility>

static_assert (CHAR_BIT == 8);
// The following two global checks ensure correct float-PCM encoding/decoding,
// avoiding a larger, more distributed set of path-specific checks.
// The trade-off is reduced portability: builds are rejected on platforms
// with non-standard float/double widths, even where linear PCM encoding/decoding
// would behave acceptably.
static_assert (sizeof (float) == 4 && std::numeric_limits<float>::is_iec559);
static_assert (sizeof (double) == 8 && std::numeric_limits<double>::is_iec559);

namespace wav
{

enum class ExportFormat
{
    LinearPCM_8,
    LinearPCM_16,
    LinearPCM_24,
    LinearPCM_32,
    FloatPCM_32,
    FloatPCM_64
};

namespace FormatCode // format code in "fmt " chunk
{
inline constexpr uint16_t LinearPCM  = 0x0001;
inline constexpr uint16_t FloatPCM   = 0x0003;
inline constexpr uint16_t Extensible = 0xFFFE;
}

constexpr bool isValidFormatCode (uint16_t formatCode);

constexpr std::pair <uint16_t, uint16_t> bitDepthAndFormatCode (ExportFormat ef);

namespace FormatChunkSize
{
inline constexpr uint32_t WAVEFORMAT           = 14;
inline constexpr uint32_t PCMWAVEFORMAT        = 16; // 14+2      (adds bitsPerSample)
inline constexpr uint32_t WAVEFORMATEX         = 18; // 14+2+2    (adds cbSize)
inline constexpr uint32_t WAVEFORMATEXTENSIBLE = 40; // 14+2+2+22 (adds validBitsPerSample, channel bit mask, and sub-format GUID)
}

constexpr bool isValidFormatChunkSize (uint32_t chunkSize);

struct GUID // for WAVEFORMATEXTENSIBLE
{
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    std::array<uint8_t,8> data4;

    auto operator<=> (GUID const &) const = default; // allows for comparing two GUIDs
    std::string str() const;
};

namespace ExtensibleFormat // WAVEFORMATEXTENSIBLE sub-formats
{
inline constexpr GUID LinearPCM
{0x00000001, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
inline constexpr GUID FloatPCM
{0x00000003, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
}

inline std::ostream & operator<< (std::ostream & os, GUID const & guid);

// Channel mask bits for WAVEFORMATEXTENSIBLE.
// Channels are interleaved in bit order (LSB first).
// The channel count and number of assigned speaker positions
// need not be equal: the channel mask is written as given.
namespace SpeakerPosition
{
inline constexpr uint32_t FrontLeft          = 0x00000001;
inline constexpr uint32_t FrontRight         = 0x00000002;
inline constexpr uint32_t FrontCenter        = 0x00000004;
inline constexpr uint32_t LowFrequency       = 0x00000008;
inline constexpr uint32_t BackLeft           = 0x00000010;
inline constexpr uint32_t BackRight          = 0x00000020;
inline constexpr uint32_t FrontLeftOfCenter  = 0x00000040;
inline constexpr uint32_t FrontRightOfCenter = 0x00000080;
inline constexpr uint32_t BackCenter         = 0x00000100;
inline constexpr uint32_t SideLeft           = 0x00000200;
inline constexpr uint32_t SideRight          = 0x00000400;
inline constexpr uint32_t TopCenter          = 0x00000800;
inline constexpr uint32_t TopFrontLeft       = 0x00001000;
inline constexpr uint32_t TopFrontCenter     = 0x00002000;
inline constexpr uint32_t TopFrontRight      = 0x00004000;
inline constexpr uint32_t TopBackLeft        = 0x00008000;
inline constexpr uint32_t TopBackCenter      = 0x00010000;
inline constexpr uint32_t TopBackRight       = 0x00020000;
inline constexpr uint32_t ValidBits          = 0x0003FFFF;
}

// Some common speaker configs to save some typing.
namespace SpeakerConfiguration
{
inline constexpr uint32_t Unassigned {0};
inline constexpr uint32_t Stereo
{
    wav::SpeakerPosition::FrontLeft |
    wav::SpeakerPosition::FrontRight
};
inline constexpr uint32_t Quadraphonic
{
    wav::SpeakerPosition::FrontLeft    |
    wav::SpeakerPosition::FrontRight   |
    wav::SpeakerPosition::BackLeft     |
    wav::SpeakerPosition::BackRight
};
inline constexpr uint32_t Five_Point_One
{
    wav::SpeakerPosition::FrontLeft    |
    wav::SpeakerPosition::FrontRight   |
    wav::SpeakerPosition::FrontCenter  |
    wav::SpeakerPosition::LowFrequency |
    wav::SpeakerPosition::SideLeft     |
    wav::SpeakerPosition::SideRight
};
inline constexpr uint32_t Seven_Point_One
{
    wav::SpeakerPosition::FrontLeft    |
    wav::SpeakerPosition::FrontRight   |
    wav::SpeakerPosition::FrontCenter  |
    wav::SpeakerPosition::LowFrequency |
    wav::SpeakerPosition::BackLeft     |
    wav::SpeakerPosition::BackRight    |
    wav::SpeakerPosition::SideLeft     |
    wav::SpeakerPosition::SideRight
};

static_assert (Unassigned      == 0x0000);
static_assert (Stereo          == 0x0003);
static_assert (Quadraphonic    == 0x0033);
static_assert (Five_Point_One  == 0x060F);
static_assert (Seven_Point_One == 0x063F);
}


template <typename T>
concept FloatOrDouble = std::same_as <T,float> || std::same_as <T,double>;

} // namespace wav

#include "private/FormatsImplementation.hpp"

#endif
