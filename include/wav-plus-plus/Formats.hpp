
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

constexpr std::pair <uint16_t, uint16_t> bitDepthAndFormatCode (ExportFormat ef) noexcept;

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

template <typename T>
concept FloatOrDouble = std::same_as <T,float> || std::same_as <T,double>;

} // namespace wav

#include "private/FormatsImplementation.hpp"

#endif
