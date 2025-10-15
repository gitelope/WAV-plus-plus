
#include "../shared/Reader.hpp"
#include "../shared/Writer.hpp"

#include <vector>
#include <iostream>
#include <format>
#include <cassert>
#include <array>

template <typename T> void Print (std::vector<T> const &);
template <typename T> void Print (std::vector<std::vector<T>> const &);


int main ()
{
    const auto filepath = "out.wav";
    const std::vector<double> out {-1, -0.5, -1/3., 0, 1/3., 0.5, 1, 0};

    wav::Writer writer {filepath, wav::ExportFormat::FloatPCM_64, 48000, 2};
    const auto samplesWritten = writer.writeInterleaved (out.data(), out.size());
    writer.Close(); // Close() would be called automatically if Writer were to go out-of-scope.

    assert (samplesWritten == out.size());

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
