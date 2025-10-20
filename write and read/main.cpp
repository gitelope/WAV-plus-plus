
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

    // write and read entire sequence of samples all at once
    {
        const std::vector<double> out {-1, -0.5, -1./3., 0, 1./3., 0.5, 1, 0};

        wav::Writer writer {filepath, wav::ExportFormat::LinearPCM_24, 48000, 2};
        const auto samplesWritten = writer.write (out);
        writer.Close(); // would be called automatically if Writer were to go out-of-scope.

        assert (samplesWritten == out.size());

        wav::Reader reader {filepath};
        std::vector<double> in (reader.samples());
        reader.read (in);

        Print (in); std::cout << std::endl;
    }

    // incremental reads and writes
    {
        wav::Writer writer {filepath, wav::ExportFormat::FloatPCM_32, 48000, 1};
        std::vector<float> buffer;
        // 2 writes
        buffer = {1.f, 2.f, 3.f}; writer.write (buffer);
        buffer = {4.f, 5.f, 6.f}; writer.write (buffer);
        writer.Close();

        wav::Reader reader {filepath};
        std::vector<float> in (2);
        // 3 reads
        reader.read (in); Print (in);
        reader.read (in); Print (in);
        reader.read (in); Print (in);
    }

    return 0;
}


template <typename T>
void Print (std::vector<T> const & interleaved)
{
    for (size_t i=0; i < interleaved.size(); ++i)
    {
        std::cout << std::format ("{}", interleaved[i]);
        if (i < interleaved.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
}

template <typename T>
void Print (std::vector<std::vector<T>> const & deinterleaved)
{
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
