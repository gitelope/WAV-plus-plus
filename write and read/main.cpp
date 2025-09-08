
#include "../shared/Read.hpp"
#include "../shared/Write.hpp"

#include <vector>
#include <iostream>
#include <format>
#include <cassert>
#include <array>

// 1. Write with wav::Write.
// 2. Read with
//    a) wav::ReadInterleaved
//    b) wav::Read (de-interleaved version)

template <typename T> void Print (std::vector<T> const &);
template <typename T> void Print (std::vector<std::vector<T>> const &);


int main (int argc, const char * argv[])
{
    // wav::Write() is overloaded to work with std::vector, C-style arrays, and std::array.
    // Templated to accept floats or doubles.
    //
    // const std::array<float, 5> out {-1, -0.5, 0, 0.5, 1}; // std::array
    // const double out[] {-1, -0.5, 0, 0.5, 1};             // C-style array
    const std::vector<double> out {-1, -0.5, 0, 0.5, 1};     // std::vector

    const auto filepath = "out.wav";

    wav::Write (filepath, out, wav::ExportFormat::LinearPCM_24, 48000, 1);

    // Data can be read in an interleaved format or with channel data separated,
    // as either floats or doubles.
    const auto [interleaved,   info1] = wav::ReadInterleaved<float> (filepath);
    const auto [deinterleaved, info2] = wav::Read<double> (filepath);

    std::cout << '\n' << filepath << "\n\n" << info1 << std::endl;

    assert (interleaved.size() == std::ranges::size (out));

    Print (interleaved);
    Print (deinterleaved);

    return 0;
}


template <typename T>
void Print (std::vector<T> const & interleaved)
{
    std::cout << "interleaved:\n";
    for (auto const & sample : interleaved)
    {
        std::cout << std::format ("{}, ", sample);
    }
    std::cout << '\n' << std::endl;
}

template <typename T>
void Print (std::vector<std::vector<T>> const & deinterleaved)
{
    std::cout << "de-interleaved:\n";
    for (auto const & channel : deinterleaved)
    {
        for (auto const & sample : channel)
        {
            std::cout << std::format ("{}, ", sample);
        }
        std::cout << '\n';
    }
    std::cout << std::endl;
}
