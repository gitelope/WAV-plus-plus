
#ifndef formats_hpp
#define formats_hpp

#include <cstdint>
#include <string>
#include <array>

namespace wav
{

enum class ExportFormat
{
    LINEAR_PCM_8,
    LINEAR_PCM_16,
    LINEAR_PCM_24,
    LINEAR_PCM_32,
    FLOAT_PCM_32,
    FLOAT_PCM_64
};

namespace FormatCode // format code in "fmt " chunk
{
static constexpr uint16_t LINEAR_PCM = 0x0001;
static constexpr uint16_t FLOAT_PCM  = 0x0003;
static constexpr uint16_t EXTENSIBLE = 0xFFFE;
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
static constexpr GUID LINEAR_PCM
{0x00000001, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
static constexpr GUID FLOAT_PCM
{0x00000003, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
}

std::ostream & operator<< (std::ostream & os, GUID const & guid);

} // namespace wav

#endif
