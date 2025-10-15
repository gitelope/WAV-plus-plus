
#ifndef wavplusplus_Formats_hpp
#define wavplusplus_Formats_hpp

#include <array>
#include <concepts>
#include <cstdint>
#include <format>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>

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
inline constexpr uint16_t LinearPCM = 0x0001;
inline constexpr uint16_t FloatPCM  = 0x0003;
inline constexpr uint16_t Extensible = 0xFFFE;
}

constexpr bool isValidFormatCode (uint32_t formatCode)
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

namespace FormatChunkSize
{
inline constexpr uint32_t WAVEFORMAT           = 14;
inline constexpr uint32_t PCMWAVEFORMAT        = 16; // 14+2      (adds bitsPerSample)
inline constexpr uint32_t WAVEFORMATEX         = 18; // 14+2+2    (adds cbSize, here renamed extraFormatInformationSize)
inline constexpr uint32_t WAVEFORMATEXTENSIBLE = 40; // 14+2+2+22 (adds validBitsPerSample, channel bit mask, and sub-format GUID)
}

constexpr bool isValidFormatChunkSize (uint32_t chunkSize)
{
    return
    chunkSize == FormatChunkSize::WAVEFORMAT         ||
    chunkSize == FormatChunkSize::PCMWAVEFORMAT      ||
    chunkSize == FormatChunkSize::WAVEFORMATEX       ||
    chunkSize == FormatChunkSize::WAVEFORMATEXTENSIBLE;
}

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

template <typename T>
concept FloatOrDouble = std::same_as <T,float> || std::same_as <T,double>;

} // namespace wav

#endif
