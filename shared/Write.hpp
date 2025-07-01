
#ifndef wavplusplus_Write_hpp
#define wavplusplus_Write_hpp

#include "WaveFile.hpp"
#include <vector>

/*
 encoding options:
 linear PCM: 8-bit, 16-bit, 24-bit, and 32-bit
 float PCM: 32-bit and 64-bit

 channel options:
 mono, stereo, and multichannel

 format:
 PCMWAVEFORMAT (very common, but technically non-standard for some of the above options)
 */

namespace wav
{

void Normalize (std::vector<double> &);
void Write (std::vector<double> const & samples, ExportFormat format, int sample_rate, int channels, std::string const & filepath);

} // namespace wav

#endif
