
#ifndef wavplusplus_Formats_hpp
#define wavplusplus_Formats_hpp

#include <cstdint>
#include <string>
#include <array>

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

std::ostream & operator<< (std::ostream & os, GUID const & guid);

template <typename T>
concept FloatOrDouble = std::same_as <T,float> || std::same_as <T,double>;

} // namespace wav

#endif
