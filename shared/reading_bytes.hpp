
#ifndef wavplusplus_reading_bytes_hpp
#define wavplusplus_reading_bytes_hpp

#include <vector>
#include <string>

namespace wav
{

struct GUID;

void ReadBytes (std::string const & filePath, std::vector<std::byte> &);
void PrintBytesAsHex (std::vector<std::byte> const & bytes);

// These read functions convert bytes to the desired type, return it,
// AND increment the read index.
//
// The "le" suffix signifies a little-endian encoding in the WAV file.
//
// It's assumed the OS uses a little endian representation for arithmetic types.
// With C++20's inclusion of std::endian, the code could be modified to support
// big-endian systems with compile-time branching using constexpr if.

// header and data
uint8_t     uint8             (std::vector<std::byte> const &, int & pos); // 1 byte   -> 8-bit  unsigned int
uint8_t     uint8             (std::ifstream &, int & pos);

// header
uint16_t    uint16_le         (std::vector<std::byte> const &, int & pos); // 2 bytes  -> 16-bit unsigned int
uint16_t    uint16_le         (std::ifstream &, int & pos);
uint32_t    uint32_le         (std::vector<std::byte> const &, int & pos); // 4 bytes  -> 32-bit unsigned int
uint32_t    uint32_le         (std::ifstream &, int & pos);
std::string fourCharacterCode (std::vector<std::byte> const &, int & pos); // 4 bytes  -> 4-character string
std::string fourCharacterCode (std::ifstream &, int & pos);
GUID        guid              (std::vector<std::byte> const &, int & pos); // 16 bytes -> GUID struct (WAVEFORMATEXTENSIBLE only)
GUID        guid              (std::ifstream &, int & pos);

// data
int16_t     int16_le          (std::vector<std::byte> const &, int & pos); // 2 bytes -> 16-bit signed int
int16_t     int16_le          (std::ifstream &, int & pos);
int32_t     int24_le          (std::vector<std::byte> const &, int & pos); // 3 bytes -> 32-bit signed int
int32_t     int24_le          (std::ifstream &, int & pos);
int32_t     int32_le          (std::vector<std::byte> const &, int & pos); // 4 bytes -> 32-bit signed int
int32_t     int32_le          (std::ifstream &, int & pos);
float       float32_le        (std::vector<std::byte> const &, int & pos); // 4 bytes -> 32-bit float
float       float32_le        (std::ifstream &, int & pos);
double      float64_le        (std::vector<std::byte> const &, int & pos); // 8 bytes -> 64-bit double
double      float64_le        (std::ifstream &, int & pos);

void dequantize (std::vector<int32_t> const & integers, int bit_depth, std::vector<double> & doubles);

} // namespace wav

#endif
