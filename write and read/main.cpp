
#include "../shared/Read.hpp"
#include "../shared/Write.hpp"
#include "../shared/Reader.hpp"
#include "../shared/Writer.hpp"

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


int main ()
{
    // wav::Write() is overloaded to work with std::vector, C-style arrays, and std::array
    // and templated to accept floats or doubles.
    //
    // const std::array<float, 5> out {-1, -0.5, 0, 0.5, 1}; // std::array
    // const double out[] {-1, -0.5, 0, 0.5, 1};             // C-style array
    const std::vector<double> out {-1, -0.5, -1/3., 0, 1/3., 0.5, 1, 0};     // std::vector

    const auto filepath = "out.wav";

    // wav::Write (filepath, out, wav::ExportFormat::FloatPCM_64, 48000, 2);

    wav::Writer writer {filepath, wav::ExportFormat::FloatPCM_64, 48000, 2};
    auto samplesWritten = writer.writeInterleaved (out.data(), out.size());
    writer.Close(); // Close() would be called automatically if Writer were to go out-of-scope.

    assert (samplesWritten == out.size());

    // Data can be read in as either floats or doubles in an interleaved format
    // or with channel data separated.
    const auto [interleaved,   info1] = wav::ReadInterleaved<double> (filepath);
    const auto [deinterleaved, info2] = wav::ReadSeparateChannels<double> (filepath);

    std::cout << '\n' << filepath << "\n\n" << info1 << std::endl;

    assert (interleaved.size() == std::ranges::size (out));

    Print (interleaved);
    Print (deinterleaved);

    wav::Reader reader {filepath};
    const auto & info {reader.getInfo()};
    std::vector<float> in (info.frames * info.channels);
    reader.readInterleaved (in.data(), in.size());

    Print (in);

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
