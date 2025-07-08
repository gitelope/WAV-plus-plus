
#include "../shared/Read.hpp"
#include "../shared/Write.hpp"

#include <vector>
#include <iostream>
#include <format>
#include <cassert>

// 1. Write with wav::Write.
// 2. Read with wav::Read.
// 3. (optional) Print samples.

int main (int argc, const char * argv[])
{
    const std::vector<double> out {-1, 0, 1, 0};
    const auto filepath = "out.wav";
    wav::Write (out, wav::ExportFormat::LINEAR_PCM_24, 48000, 1, filepath);

    wav::Info info;
    std::vector<double> in;
    wav::Read (filepath, in, info);

    std::cout << '\n' << filepath << "\n\n" << info << std::endl;

    assert (in.size() == out.size());

    const bool printSamples {true};
    if (printSamples)
    {
        for (auto sample : in) std::cout << std::format("{}", sample) << ",";
        std::cout << '\n' << std::endl;
    }

    return 0;
}
