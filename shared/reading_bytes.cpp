
#include "reading_bytes.hpp"
#include "WaveFile.hpp"
#include <fstream>
#include <cassert>
#include <iostream>

namespace wav
{

void ReadBytes (std::string const & filePath, std::vector<std::byte> & bytes)
{
    std::ifstream in {filePath, std::ios::binary | std::ios::in};
    const auto file_length = std::filesystem::file_size (filePath);
    bytes.resize (file_length);
    in.read (reinterpret_cast<char *>(bytes.data()), file_length);
    in.close();
}

void PrintBytesAsHex (std::vector<std::byte> const & bytes)
{
    // used to reset cout when done:
    std::ios_base::fmtflags default_cout_flags (std::cout.flags());

    std::cout << std::hex << std::setfill('0');
    for (auto const & byte : bytes)
    {
        std::cout << std::setw(2) << std::to_integer<int>(byte) << " ";
    }
    std::cout << std::endl;

    // reset cout
    std::cout.flags (default_cout_flags);
}

std::string fourCharacterCode (std::vector<std::byte> const & bytes, int & pos)
{
    const auto code =  std::string{}
    + static_cast<char>(bytes[pos])
    + static_cast<char>(bytes[pos+1])
    + static_cast<char>(bytes[pos+2])
    + static_cast<char>(bytes[pos+3]);

    pos += 4;

    return code;
}

std::string fourCharacterCode (std::ifstream & in)
{
    std::array<char, 4> buffer {};
    in.read (buffer.data(), buffer.size());
    return std::string (buffer.data(), buffer.size());
}

uint8_t uint8 (std::vector<std::byte> const & bytes, int & pos)
{
    return static_cast<uint8_t> (bytes [pos++]);
}

uint8_t uint8 (std::ifstream & in)
{
    uint8_t result {0};
    in.read (reinterpret_cast<char*> (&result), 1);
    return result;
}

GUID guid (std::vector<std::byte> const & bytes, int & pos)
{
    const uint32_t data1 = uint32_le (bytes, pos);
    const uint16_t data2 = uint16_le (bytes, pos);
    const uint16_t data3 = uint16_le (bytes, pos);
    std::array<uint8_t,8> data4;
    for (int i=0; i<8; ++i) data4[i] = uint8 (bytes, pos);
    return {data1, data2, data3, data4};
}

GUID guid (std::ifstream & in)
{
    const auto data1 {uint32_le (in)};
    const auto data2 {uint16_le (in)};
    const auto data3 {uint16_le (in)};
    std::array <uint8_t, 8> data4;
    for (int i=0; i<data4.size(); ++i) data4[i] = uint8 (in);
    return {data1, data2, data3, data4};
}

uint32_t uint32_le (std::vector<std::byte> const & bytes, int & pos)
{
    const auto result =
    static_cast<uint32_t>(bytes[pos])                 // 2^0
    + static_cast<uint32_t>(bytes[pos+1]) * 256       // 2^8
    + static_cast<uint32_t>(bytes[pos+2]) * 65536     // 2^16
    + static_cast<uint32_t>(bytes[pos+3]) * 16777216; // 2^24

    /*
     // This does the same with bitshifting and bitwise OR:

     const auto result_alt =
     static_cast<uint32_t>(bytes[pos+3]) << 24
     | static_cast<uint32_t>(bytes[pos+2]) << 16
     | static_cast<uint32_t>(bytes[pos+1]) << 8
     | static_cast<uint32_t>(bytes[pos]);

     assert (result_alt == result);
     */

    pos += 4;

    return result;
}

uint32_t uint32_le (std::ifstream & in)
{
    std::array <uint8_t, 4> buffer {};
    in.read (reinterpret_cast<char*> (buffer.data()), buffer.size());

    return
    static_cast<uint32_t> (buffer[0])       |
    static_cast<uint32_t> (buffer[1]) << 8  |
    static_cast<uint32_t> (buffer[2]) << 16 |
    static_cast<uint32_t> (buffer[3]) << 24 ;
}

uint16_t uint16_le (std::vector<std::byte> const & bytes, int & pos)
{
    const auto result =
    static_cast<uint16_t>(bytes[pos])            // 2^0
    + static_cast<uint16_t>(bytes[pos+1]) * 256; // 2^8

    /*
     // This does the same with bitshifting and bitwise OR

     const auto result_alt =
     static_cast<uint32_t>(bytes[pos+1]) << 8
     | static_cast<uint32_t>(bytes[pos]);

     assert (result_alt == result);
     */

    pos += 2;

    return result;
}

uint16_t uint16_le (std::ifstream & in)
{
    std::array <uint8_t, 2> buffer {};
    in.read (reinterpret_cast<char*> (buffer.data()), buffer.size());

    return
    static_cast<uint32_t> (buffer [0])     |
    static_cast<uint32_t> (buffer [1]) << 8;
}

int16_t int16_le (std::vector<std::byte> const & bytes, int & pos)
{
    const auto result =
    static_cast<int16_t> (bytes [pos])       |
    static_cast<int16_t> (bytes [pos+1]) << 8;

    pos += 2;

    return result;
}

int16_t int16_le (std::ifstream & in)
{
    std::array <uint8_t, 2> buffer {};
    in.read (reinterpret_cast<char*> (buffer.data()), buffer.size());

    return
    static_cast<int16_t> (buffer [0])       |
    static_cast<int16_t> (buffer [1])   << 8;
}

int32_t int24_le (std::vector<std::byte> const & bytes, int & pos)
{
    // 24-bit signed int --> 32-bit signed int
    // (both 2's-complement)
    //
    // Clear the most significant bit of the most significant byte.
    // Cast the bytes to signed 32-bit ints.
    // Bit shift the ints to the left according to the significance of their corresponding bytes.
    // If most significant bit (of original MSB) is set, subtract 2^23 from the result.
    //
    // 0x7F: 0b01111111
    // 0x80: 0b10000000
    const auto result =
    (
       static_cast<int32_t> (bytes [pos+2] & std::byte{0x7F}) << 16 |
       static_cast<int32_t> (bytes [pos+1])                   << 8  |
       static_cast<int32_t> (bytes [pos])
    )
    - (static_cast<int32_t> (bytes [pos+2] & std::byte{0x80}) << 16);

    pos += 3;

    return result;
}

int32_t int24_le (std::ifstream & in)
{
    // 24-bit signed int --> 32-bit signed int
    // (both 2's-complement)
    //
    // Clear the most significant bit of the most significant byte.
    // Cast the bytes to signed 32-bit ints.
    // Bit shift the ints to the left according to the significance of their corresponding bytes.
    // If most significant bit (of original MSB) is set, subtract 2^23 from the result.
    //
    // 0x7F: 0b01111111
    // 0x80: 0b10000000
    std::array <uint8_t, 3> buffer {};
    in.read (reinterpret_cast<char*> (buffer.data()), buffer.size());

    return
    (
       static_cast<int32_t>   (buffer[2] & 0x7F) << 16 |
       static_cast<int32_t>   (buffer[1])        << 8  |
       static_cast<int32_t>   (buffer[0])
    )
    - (static_cast<int32_t>   (buffer[2] & 0x80) << 16);
}

int32_t int32_le (std::vector<std::byte> const & bytes, int & pos)
{
    const auto result =
    static_cast<int32_t> (bytes [pos+3]) << 24 |
    static_cast<int32_t> (bytes [pos+2]) << 16 |
    static_cast<int32_t> (bytes [pos+1]) << 8  |
    static_cast<int32_t> (bytes [pos])         ;

    pos += 4;

    return result;
}

int32_t int32_le (std::ifstream & in)
{
    std::array <uint8_t, 4> buffer {};
    in.read (reinterpret_cast<char*> (buffer.data()), buffer.size());

    return
    static_cast<int32_t> (buffer [3]) << 24 |
    static_cast<int32_t> (buffer [2]) << 16 |
    static_cast<int32_t> (buffer [1]) << 8  |
    static_cast<int32_t> (buffer [0])       ;
}

float float32_le (std::vector<std::byte> const & bytes, int & pos)
{
    /*
     const auto result =
     static_cast<int32_t>(bytes[pos+3]) << 24
     | static_cast<int32_t>(bytes[pos+2]) << 16
     | static_cast<int32_t>(bytes[pos+1]) << 8
     | static_cast<int32_t>(bytes[pos]);

     pos += 4;
     */

    int32_t integer = 0;
    for (int n=0; n<4; ++n) integer |= (static_cast<int32_t>(bytes[pos++]) << 8 * n);
    return std::bit_cast<float> (integer);
}

float float32_le (std::ifstream & in)
{
    /*
     const auto result =
     static_cast<int32_t>(bytes[pos+3]) << 24
     | static_cast<int32_t>(bytes[pos+2]) << 16
     | static_cast<int32_t>(bytes[pos+1]) << 8
     | static_cast<int32_t>(bytes[pos]);

     pos += 4;
     */

    std::array <uint8_t, 4> buffer {};
    in.read (reinterpret_cast<char*> (buffer.data()), buffer.size());

    int32_t integer = 0;
    for (int n=0; n<4; ++n) integer |= (static_cast<int32_t> (buffer [n]) << 8 * n);
    return std::bit_cast<float> (integer);
}

double float64_le (std::vector<std::byte> const & bytes, int & pos)
{
    /*
     const auto integer =
     static_cast<int64_t>(bytes[pos+7]) << 56
     | static_cast<int64_t>(bytes[pos+6]) << 48
     | static_cast<int64_t>(bytes[pos+5]) << 40
     | static_cast<int64_t>(bytes[pos+4]) << 32
     | static_cast<int64_t>(bytes[pos+3]) << 24
     | static_cast<int64_t>(bytes[pos+2]) << 16
     | static_cast<int64_t>(bytes[pos+1]) << 8
     | static_cast<int64_t>(bytes[pos]);
     pos += 8;
     */

    int64_t integer = 0;
    for (int n=0; n<8; ++n) integer |= (static_cast<int64_t> (bytes [pos++]) << 8 * n);
    return std::bit_cast<double> (integer);
}

double float64_le (std::ifstream & in)
{
    /*
     const auto integer =
     static_cast<int64_t>(bytes[pos+7]) << 56
     | static_cast<int64_t>(bytes[pos+6]) << 48
     | static_cast<int64_t>(bytes[pos+5]) << 40
     | static_cast<int64_t>(bytes[pos+4]) << 32
     | static_cast<int64_t>(bytes[pos+3]) << 24
     | static_cast<int64_t>(bytes[pos+2]) << 16
     | static_cast<int64_t>(bytes[pos+1]) << 8
     | static_cast<int64_t>(bytes[pos]);
     pos += 8;
     */

    std::array <uint8_t, 8> buffer {};
    in.read (reinterpret_cast<char*> (buffer.data()), buffer.size());

    int64_t integer = 0;
    for (int n=0; n<8; ++n) integer |= (static_cast<int64_t> (buffer [n]) << 8 * n);
    return std::bit_cast<double> (integer);
}

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

} // namespace wav
