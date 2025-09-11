
#ifndef wavplusplus_ReadingBytes_hpp
#define wavplusplus_ReadingBytes_hpp

#include "Formats.hpp"
#include "Exceptions.hpp"
#include <vector>
#include <string>
#include <cstdio>
#include <filesystem>
#include <memory>

namespace wav
{

// These read functions convert bytes to the desired type, return it, and
// either increment the read position for the byte vector
// or advance the FILE pointer.
//
// The "le" suffix signifies a little-endian encoding in the WAV file.
std::string fourCharacterCode (FILE *); // 4 bytes  -> 4-character string
GUID        guid              (FILE *); // 16 bytes -> GUID struct (WAVEFORMATEXTENSIBLE only)
//
// unsigned int
uint8_t     uint8             (std::vector<std::byte> const &, size_t & pos); // 1 byte   -> 8-bit  unsigned int
uint8_t     uint8             (FILE *);
uint16_t    uint16_le         (FILE *);                              // 2 bytes  -> 16-bit unsigned int
uint32_t    uint32_le         (FILE *);                              // 4 bytes  -> 32-bit unsigned int
//
// signed int
int16_t     int16_le          (std::vector<std::byte> const &, size_t & pos); // 2 bytes -> 16-bit signed int
int32_t     int24_le          (std::vector<std::byte> const &, size_t & pos); // 3 bytes -> 32-bit signed int
int32_t     int32_le          (std::vector<std::byte> const &, size_t & pos); // 4 bytes -> 32-bit signed int
//
// floating point
float       float32_le        (std::vector<std::byte> const &, size_t & pos); // 4 bytes -> 32-bit float
float       float32_le        (FILE *);
double      float64_le        (std::vector<std::byte> const &, size_t & pos); // 8 bytes -> 64-bit double

inline auto fileForReading (std::string const & filepath)
{
    if (! std::filesystem::is_regular_file (filepath))
    {
        throw ReadError ("File does not exist: " + filepath);
    }
    // Open FILE and ensure that attempt is made to close file if an exception is thrown.
    // (fclose failure during stack unwinding is ignored to avoid throwing another exception
    // and thus a call to std::terminate.)
    return std::unique_ptr<FILE, decltype (&fclose)> (fopen (filepath.c_str(), "rb"), &fclose);
}

void Close (std::unique_ptr<FILE, decltype (&fclose)> & filePointer);

int peek (FILE * in); // check if there's data available

std::vector<std::byte> ReadBytes (std::string const & filePath);
void PrintBytesAsHex (std::vector<std::byte> const & bytes);

// void dequantize (std::vector<int32_t> const & integers, int bit_depth, std::vector<double> & doubles);

} // namespace wav

#endif
