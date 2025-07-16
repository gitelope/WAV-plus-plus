
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
    const std::vector<double> out {-1, 1, 0, 0};
    const auto filepath = "out.wav";
    wav::Write (out, wav::ExportFormat::LINEAR_PCM_24, 48000, 2, filepath);

    const auto [interleaved,   info]  = wav::ReadInterleaved<double> (filepath);
    const auto [deinterleaved, info2] = wav::Read<double> (filepath);

    std::cout << '\n' << filepath << "\n\n" << info << std::endl;

    assert (interleaved.size() == out.size());

    const bool printSamples {true};
    if (printSamples)
    {
        std::cout << "interleaved:\n";
        for (auto const & sample : interleaved)
        {
            std::cout << std::format("{}", sample) << ",";
        }
        std::cout << '\n' << std::endl;

        std::cout << "de-interleaved:\n";
        for (auto const & channel : deinterleaved)
        {
            for (auto const & sample : channel)
            {
                std::cout << std::format("{}", sample) << ",";
            }
            std::cout << '\n';
        }
        std::cout << std::endl;
    }

    return 0;
}
