
#ifndef wavplusplus_Write_hpp
#define wavplusplus_Write_hpp

#include "Formats.hpp"
#include "Exceptions.hpp"

#include <vector>
#include <string>
#include <fstream>
#include <cassert>

/*

 sample encoding options:
 linear PCM: 8-bit, 16-bit, 24-bit, and 32-bit
 float PCM: 32-bit and 64-bit

 channel options:
 mono, stereo, and multichannel

 format:
 PCMWAVEFORMAT (very common, but technically non-standard for some of the above options)

 ---

 quantization (linear PCM only):

 double -> signed int (16, 24 and 32 bits):
 1. Multiply by 2^(bits-1) - 1
    (e.g. 32,767 for 16-bit linear PCM)
 2. Round to nearest integer.*
 3. Clamp to [1 - 2^(bits-1), 2^(bits-1) - 1]
    (e.g. [-32,767, 32,767] for 16-bit linear PCM)

double -> unsigned 8-bit int:
 1. Multiply by 127
 2. Add 128.
 3. Round to nearest integer.*
 4. Clamp to [1, 255].

 * Optional TPDF dither (not implemented) would come before rounding.

 */

namespace wav
{

// May throw wav::WriteError.
void Write (std::vector<double> const & samples,
            ExportFormat format,
            int sampleRate,
            int channels,
            std::string const & filepath);

void Normalize (std::vector<double> &);

////////////////////////////////////////////////////////
// HELPER FUNCTIONS
// (may throw wav::WriteError)

void WriteData (std::ofstream & out,
                std::vector<double> const & samples,
                uint16_t bits_per_sample,
                uint16_t format_code);

// Write string.
void WriteToFile (std::ofstream & out, std::string const & str);

// Write number.
//
// By default, the number of bytes is the container size,
// but it may also be less than the container size.
template <typename T>
requires (std::integral<T> || std::floating_point<T>)
void WriteToFile (std::ofstream & out, T value, int size = sizeof(value))
{
    assert (size > 0 && size <= sizeof(value));
    out.write (reinterpret_cast<const char *> (&value), size);
    if (!out)
    {
        throw WriteError ("Failed to write to file.");
    }
}

double clamped (double value, double min, double max);

} // namespace wav

#endif
