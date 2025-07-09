
#include "ReadingBytes.hpp"
#include <fstream>
#include <cassert>
#include <iostream>

namespace wav
{

std::string fourCharacterCode (std::ifstream & in)
{
    std::string code (4, '\0');
    in.read (code.data(), 4);
    if (!in) throw std::ios_base::failure ("Failed to read 4 characters from stream.");
    return code;
}

GUID guid (std::ifstream & in)
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

// Helper for functions that read directly from a std::ifstream.
template <typename T, size_t N>
void read (std::istream & in, std::array<T,N> & buffer)
{
    if (!in.read (reinterpret_cast<char*> (buffer.data()), buffer.size()))
    {
        throw std::ios_base::failure (std::format ("Failed to read {} bytes from stream.", N));
    }
}

uint8_t uint8 (std::vector<std::byte> const & bytes, size_t & pos)
{
    if (pos + 1 > bytes.size())
    {
        throw std::out_of_range ("Attempting to read past end of vector.");
    }
    return static_cast<uint8_t> (bytes [pos++]);
}

uint8_t uint8 (std::ifstream & in)
{
    uint8_t result {0};
    if (!in.read (reinterpret_cast<char*> (&result), 1))
    {
        throw std::ios_base::failure ("Failed to read uint8_t from ifstream.");
    }
    return result;
}

uint16_t uint16_le (std::ifstream & in)
{
    std::array <uint8_t, 2> buffer {};
    read (in, buffer);
    return
    static_cast<uint16_t> (buffer [0])     |
    static_cast<uint16_t> (buffer [1]) << 8;
}

uint32_t uint32_le (std::ifstream & in)
{
    std::array<uint8_t, 4> buffer{};
    read (in, buffer);
    return
    static_cast<uint32_t> (buffer[0])       |
    static_cast<uint32_t> (buffer[1]) << 8  |
    static_cast<uint32_t> (buffer[2]) << 16 |
    static_cast<uint32_t> (buffer[3]) << 24 ;
}

int16_t int16_le (std::vector<std::byte> const & bytes, size_t & pos)
{
    if (pos + 2 > bytes.size())
    {
        throw std::out_of_range ("Not enough bytes to read int16_t.");
    }
    const auto result =
    static_cast<int16_t> (bytes [pos])       |
    static_cast<int16_t> (bytes [pos+1]) << 8;

    pos += 2;

    return result;
}

int32_t int24_le (std::vector<std::byte> const & bytes, size_t & pos)
{
    // converts 3 bytes representing a 24-bit signed int (2's-complement)
    // to a 32-bit signed int (2's-complement) with the same value
    if (pos + 3 > bytes.size())
    {
        throw std::out_of_range ("Not enough bytes to read 24-bit int.");
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

int32_t int32_le (std::vector<std::byte> const & bytes, size_t & pos)
{
    if (pos + 4 > bytes.size())
    {
        throw std::out_of_range ("Not enough bytes to read int32_t.");
    }
    const auto result =
    static_cast<int32_t> (bytes [pos])         |
    static_cast<int32_t> (bytes [pos+1]) << 8  |
    static_cast<int32_t> (bytes [pos+2]) << 16 |
    static_cast<int32_t> (bytes [pos+3]) << 24 ;

    pos += 4;

    return result;
}

float float32_le (std::vector<std::byte> const & bytes, size_t & pos)
{
    if (pos + 4 > bytes.size())
    {
        throw std::out_of_range ("Not enough bytes to read 32-bit float.");
    }
    const auto integer =
    static_cast<uint32_t> (bytes[pos])         |
    static_cast<uint32_t> (bytes[pos+1]) << 8  |
    static_cast<uint32_t> (bytes[pos+2]) << 16 |
    static_cast<uint32_t> (bytes[pos+3]) << 24 ;

    pos += 4;

    return std::bit_cast<float> (integer);
}

float float32_le (std::ifstream & in)
{
    std::array <uint8_t, 4> buffer{};
    read (in, buffer);

    const auto integer =
    static_cast<uint32_t> (buffer[0])       |
    static_cast<uint32_t> (buffer[1]) <<  8 |
    static_cast<uint32_t> (buffer[2]) << 16 |
    static_cast<uint32_t> (buffer[3]) << 24 ;

    return std::bit_cast<float> (integer);
}

double float64_le (std::vector<std::byte> const & bytes, size_t & pos)
{
    if (pos + 8 > bytes.size())
    {
        throw std::out_of_range ("Not enough bytes to read 64-bit float.");
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

std::vector<std::byte> ReadBytes (std::string const & filePath)
{
    std::ifstream in {filePath, std::ios::binary};
    if (!in) throw std::ios_base::failure ("Failed to open file.");

    const auto length = std::filesystem::file_size (filePath);
    std::vector<std::byte> bytes (length);

    if (!in.read (reinterpret_cast<char*> (bytes.data()), length))
    {
        throw std::ios_base::failure ("Failed to read file.");
    }

    return bytes;
}

void PrintBytesAsHex (std::vector<std::byte> const& bytes)
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

/*
void dequantize (std::vector<int32_t> const & integers, int bit_depth, std::vector<double> & doubles)
{
    // Divide signed integer by the absolute value of the largest negative int for the bit depth.
    // This is equal to one greater than the largest positive value for the bit depth.
    // Example: for 16-bits, the range is [-32768, 32767] so we divide by 32768.
    // This ensures that our float is within [-1.0, 1.0] regardless of how it was clamped.
    // (For what it's worth, it will never equal +1.0.)
    const double two_to_the_n_minus_one = pow (2.0, bit_depth - 1);

    doubles.resize (integers.size());

    if (bit_depth == 8)
    {
        // Requires special handling since 8-bit samples are stored as unsigned integers in WAV files.
        for (size_t i = 0; i < integers.size(); ++i)
        {
            doubles[i] = (integers[i] - 128) / two_to_the_n_minus_one;
        }
    }
    else
    {
        for (size_t i = 0; i < integers.size(); ++i)
        {
            doubles[i] = integers[i] / two_to_the_n_minus_one;
        }
    }
}
 */

} // namespace wav
