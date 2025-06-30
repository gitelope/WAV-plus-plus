
#include "../shared/Load.hpp"
#include "../shared/SaveWAV.hpp"

#include <vector>

// 1. Write with wav::SaveWAV.
// 2. Read with wav::Read.

int main (int argc, const char * argv[])
{
    const std::vector<double> out {-1, 0, 1};
    const auto filename = "out.WAV";
    wav::SaveWAV (out, wav::ExportFormat::FLOAT_PCM_64, 48000, 1, filename);

    wav::Info info;
    std::vector<double> in;
    wav::Read (filename, in, info);

    return 0;
}
