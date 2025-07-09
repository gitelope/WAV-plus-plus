
#ifndef wavplusplus_reading_bytes_hpp
#define wavplusplus_reading_bytes_hpp

#include "formats.hpp"
#include <vector>
#include <string>

namespace wav
{

// These read functions convert bytes to the desired type, return it, and
// either increment the read position for the byte vector
// or advance the ifstream pointer (handled by ifstream).
//
// The "le" suffix signifies a little-endian encoding in the WAV file.
std::string fourCharacterCode (std::ifstream &); // 4 bytes  -> 4-character string
GUID        guid              (std::ifstream &); // 16 bytes -> GUID struct (WAVEFORMATEXTENSIBLE only)
//
// unsigned int
uint8_t     uint8             (std::vector<std::byte> const &, size_t & pos); // 1 byte   -> 8-bit  unsigned int
uint8_t     uint8             (std::ifstream &);
uint16_t    uint16_le         (std::ifstream &);                              // 2 bytes  -> 16-bit unsigned int
uint32_t    uint32_le         (std::ifstream &);                              // 4 bytes  -> 32-bit unsigned int
//
// signed int
int16_t     int16_le          (std::vector<std::byte> const &, size_t & pos); // 2 bytes -> 16-bit signed int
int32_t     int24_le          (std::vector<std::byte> const &, size_t & pos); // 3 bytes -> 32-bit signed int
int32_t     int32_le          (std::vector<std::byte> const &, size_t & pos); // 4 bytes -> 32-bit signed int
//
// floating point
float       float32_le        (std::vector<std::byte> const &, size_t & pos); // 4 bytes -> 32-bit float
float       float32_le        (std::ifstream &);
double      float64_le        (std::vector<std::byte> const &, size_t & pos); // 8 bytes -> 64-bit double

std::vector<std::byte> ReadBytes (std::string const & filePath);
void PrintBytesAsHex (std::vector<std::byte> const & bytes);

// void dequantize (std::vector<int32_t> const & integers, int bit_depth, std::vector<double> & doubles);

} // namespace wav

#endif
