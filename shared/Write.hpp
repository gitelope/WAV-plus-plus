
#ifndef wavplusplus_Write_hpp
#define wavplusplus_Write_hpp

#include "Formats.hpp"
#include <vector>

/*

 sample encoding options:
 linear PCM: 8-bit, 16-bit, 24-bit, and 32-bit
 float PCM: 32-bit and 64-bit

 channel options:
 mono, stereo, and multichannel

 format:
 PCMWAVEFORMAT (very common, but technically non-standard for some of the above options)

 quantization of double to signed int:
 1. Multiply by 2^(bits-1) - 1.
 2. Round to nearest integer.*
 3. Clamp to [1 - 2^(bits-1), 2^(bits-1) - 1].

 quantization of double to unsigned 8-bit int:
 1. Multiply by 127.
 2. Add 128.
 3. Round to nearest integer.*
 4. Clamp to [1, 255].

 * Optional TPDF dither (not implemented) would come before rounding.

 */

namespace wav
{

void Normalize (std::vector<double> &);
void Write (std::vector<double> const & samples, ExportFormat format, int sample_rate, int channels, std::string const & filepath);

} // namespace wav

#endif
