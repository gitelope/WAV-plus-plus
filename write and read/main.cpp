
#include "../shared/Read.hpp"
#include "../shared/Write.hpp"

#include <vector>
#include <iostream>
#include <format>

// 1. Write with wav::Write.
// 2. Read with wav::Read.
// 3. (optional) Print samples.

int main (int argc, const char * argv[])
{
    const std::vector<double> out {-1, 0, 1};
    const auto filename = "out.WAV";
    wav::Write (out, wav::ExportFormat::FLOAT_PCM_64, 48000, 1, filename);

    wav::Info info;
    std::vector<double> in;
    wav::Read (filename, in, info, true /*verbose*/);

    const bool printSamples {true};
    if (printSamples)
    {
        for (auto sample : in) std::cout << std::format("{}", sample) << ",";
        std::cout << '\n' << std::endl;
    }


    return 0;
}
