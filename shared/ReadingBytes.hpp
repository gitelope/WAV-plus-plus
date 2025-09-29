
#ifndef wavplusplus_ReadingBytes_hpp
#define wavplusplus_ReadingBytes_hpp

#include "Formats.hpp"
#include "Exceptions.hpp"
#include <vector>
#include <string>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <format>
#include <iostream>

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


///////////////////////////////////////////////////////////


inline std::string fourCharacterCode (FILE * in)
{
    std::string code (4, '\0');
    if (fread (code.data(), 1, 4, in) != 4)
        throw ReadError ("Failed to read 4 characters from file.");
    return code;
}

inline GUID guid (FILE * in)
{
    const auto data1 {uint32_le (in)};
    const auto data2 {uint16_le (in)};
    const auto data3 {uint16_le (in)};
    std::array<uint8_t,8> data4;
    for (auto & byte : data4)
    {
        byte = uint8 (in);
    }
    return {data1, data2, data3, data4};
}

// Helper for functions that read directly from a FILE *.
template <size_t N>
void read (FILE * in, std::array<uint8_t,N> & buffer)
{
    if (fread (buffer.data(), 1, buffer.size(), in) != buffer.size())
    {
        throw ReadError (std::format ("Failed to read {} bytes from file.", N));
    }
}

inline uint8_t uint8 (std::vector<std::byte> const & bytes, size_t & pos)
{
    if (pos >= bytes.size())
    {
        throw ReadError ("Attempting to read past end of vector.");
    }
    return static_cast<uint8_t> (bytes [pos++]);
}

inline uint8_t uint8 (FILE * in)
{
    uint8_t result {0};
    if (fread (&result, 1, 1, in) != 1)
    {
        throw ReadError ("Failed to read uint8_t from file.");
    }
    return result;
}

inline uint16_t uint16_le (FILE * in)
{
    std::array<uint8_t, 2> buffer {};
    read (in, buffer);
    return static_cast<uint16_t>
    (
        static_cast<uint16_t> (buffer [0])    |
        static_cast<uint16_t> (buffer [1]) << 8
    );
}

inline uint32_t uint32_le (FILE * in)
{
    std::array<uint8_t, 4> buffer{};
    read (in, buffer);
    return
    static_cast<uint32_t> (buffer[0])       |
    static_cast<uint32_t> (buffer[1]) << 8  |
    static_cast<uint32_t> (buffer[2]) << 16 |
    static_cast<uint32_t> (buffer[3]) << 24 ;
}

inline int16_t int16_le (std::vector<std::byte> const & bytes, size_t & pos)
{
    if (pos + 1 >= bytes.size())
    {
        throw ReadError ("Not enough bytes to read int16_t.");
    }
    const auto result = static_cast<int16_t>
    (
        static_cast<int16_t> (bytes [pos])      |
        static_cast<int16_t> (bytes [pos+1]) << 8
    );

    pos += 2;

    return result;
}

inline int32_t int24_le (std::vector<std::byte> const & bytes, size_t & pos)
{
    // converts 3 bytes representing a 24-bit signed int (2's-complement)
    // to a 32-bit signed int (2's-complement) with the same value
    if (pos + 2 >= bytes.size())
    {
        throw ReadError ("Not enough bytes to read 24-bit int.");
    }

    auto result =
    static_cast<int32_t> (bytes [pos])         |
    static_cast<int32_t> (bytes [pos+1]) <<  8 |
    static_cast<int32_t> (bytes [pos+2]) << 16 ;

    // sign extension:
    // To obtain a (2's-complement) 32-bit signed int with the
    // same value as a given (2's-complement) 24-bit signed int,
    // add 8 bits, all set, to the top, when the highest bit–the sign bit–is set
    // and add 8 bits, all cleared, to the top when the highest bit is not set, e.g.
    //
    // a negative ineger:
    //          10100100 01110010 11110010 = -6,037,566
    // 11111111 10100100 01110010 11110010 = -6,037,566
    //
    // a positive integer:
    //          00100100 01110010 11110010 =  2,398,722
    // 00000000 00100100 01110010 11110010 =  2,398,722
    if (result & 0x800000)
    {
        result |= 0xFF000000;
    }

    pos += 3;

    return result;
}

inline int32_t int32_le (std::vector<std::byte> const & bytes, size_t & pos)
{
    if (pos + 3 >= bytes.size())
    {
        throw ReadError ("Not enough bytes to read int32_t.");
    }
    const auto result =
    static_cast<int32_t> (bytes [pos])         |
    static_cast<int32_t> (bytes [pos+1]) << 8  |
    static_cast<int32_t> (bytes [pos+2]) << 16 |
    static_cast<int32_t> (bytes [pos+3]) << 24 ;

    pos += 4;

    return result;
}

inline float float32_le (std::vector<std::byte> const & bytes, size_t & pos)
{
    if (pos + 3 >= bytes.size())
    {
        throw ReadError ("Not enough bytes to read 32-bit float.");
    }
    const auto integer =
    static_cast<uint32_t> (bytes[pos])         |
    static_cast<uint32_t> (bytes[pos+1]) << 8  |
    static_cast<uint32_t> (bytes[pos+2]) << 16 |
    static_cast<uint32_t> (bytes[pos+3]) << 24 ;

    pos += 4;

    return std::bit_cast<float> (integer);
}

inline float float32_le (FILE * in)
{
    std::array<uint8_t, 4> buffer{};
    read (in, buffer);

    const auto integer =
    static_cast<uint32_t> (buffer[0])       |
    static_cast<uint32_t> (buffer[1]) <<  8 |
    static_cast<uint32_t> (buffer[2]) << 16 |
    static_cast<uint32_t> (buffer[3]) << 24 ;

    return std::bit_cast<float> (integer);
}

inline double float64_le (std::vector<std::byte> const & bytes, size_t & pos)
{
    if (pos + 7 >= bytes.size())
    {
        throw ReadError ("Not enough bytes to read 64-bit float.");
    }
    const auto integer =
    static_cast<uint64_t> (bytes[pos])         |
    static_cast<uint64_t> (bytes[pos+1]) <<  8 |
    static_cast<uint64_t> (bytes[pos+2]) << 16 |
    static_cast<uint64_t> (bytes[pos+3]) << 24 |
    static_cast<uint64_t> (bytes[pos+4]) << 32 |
    static_cast<uint64_t> (bytes[pos+5]) << 40 |
    static_cast<uint64_t> (bytes[pos+6]) << 48 |
    static_cast<uint64_t> (bytes[pos+7]) << 56 ;

    pos += 8;

    return std::bit_cast<double> (integer);
}

inline void Close (std::unique_ptr<FILE, decltype (&fclose)> & filePointer)
{
    // Close the file manually so we can throw an exception if it fails.
    if (fclose (filePointer.release()) != 0)
    {
        throw ReadError ("File did not close successfully.");
    }
}

inline int peek (FILE * in)
{
    int c = fgetc (in);
    if (c != EOF)
    {
        ungetc (c, in); // Put the character back
    }
    return c;
}

inline std::vector<std::byte> ReadBytes (std::string const & filepath)
{
    auto in = fileForReading (filepath);
    if (!in) throw ReadError ("Failed to open file.");

    const auto length = std::filesystem::file_size (filepath);
    std::vector<std::byte> bytes (length);

    if (fread (bytes.data(), 1, length, in.get()) != length)
    {
        throw ReadError ("Failed to read file.");
    }

    Close (in);

    return bytes;
}

inline void PrintBytesAsHex (std::vector<std::byte> const & bytes)
{
    // used to reset cout when done
    const auto original_flags {std::cout.flags()};

    std::cout << std::hex << std::setfill ('0');

    for (auto const & byte : bytes)
    {
        std::cout << std::setw(2) << std::to_integer<uint16_t> (byte) << ' ';
    }
    std::cout << std::endl;

    // reset cout formatting
    std::cout.flags (original_flags);
}

} // namespace wav

#endif
