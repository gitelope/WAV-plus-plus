
#ifndef wavplusplus_Write_hpp
#define wavplusplus_Write_hpp

#include "WaveFile.hpp"
#include <vector>

namespace wav
{

void Normalize (std::vector<double> &);
void Write (std::vector<double> const & samples, ExportFormat format, int sample_rate, int channels, std::string const & filepath);

} // namespace wav

#endif
