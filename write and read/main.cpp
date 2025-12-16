
#include <wav-plus-plus/Reader.hpp>
#include <wav-plus-plus/Writer.hpp>

#include <vector>
#include <iostream>
#include <format>
#include <cassert>
#include <array>

template <typename T> void Print (std::vector<T> const &);
template <typename T> void Print (std::vector<std::vector<T>> const &);


int main ()
{
    const auto filename {"test.wav"};

    // Single-pass writing and reading. Displaying file metadata.
    {
        {
            const std::vector<double> out {-1, -0.5, -1./3., 0, 1./3., 0.5, 1, 0};

            wav::Writer writer {filename, wav::ExportFormat::LinearPCM_24, 48000, 2};
            writer.write (out);
        }
        {
            wav::Reader reader {filename};
            std::vector<double> in (reader.samples());
            reader.read (in);

            std::cout << "\n";
            std::cout << reader.getInfo() << std::endl;

            std::cout << "entire file read in a single read:\n";
            Print (in);
        }
    }

    // Incremental writing and reading (with tallying of samples).
    {
        size_t samplesWritten {0};
        size_t samplesRead {0};

        {
            wav::Writer writer {filename, wav::ExportFormat::FloatPCM_32, 48000, 1};
            std::vector<float> buffer;

            // 2 writes
            buffer = {1.f, 2.f, 3.f}; samplesWritten += writer.write (buffer);
            buffer = {4.f, 5.f, 6.f}; samplesWritten += writer.write (buffer);
        }
        {
            wav::Reader reader {filename};
            std::vector<float> in (2);

            // 3 reads
            std::cout << "\nfile read incrementally:\n";
            samplesRead += reader.read (in); Print (in);
            samplesRead += reader.read (in); Print (in);
            samplesRead += reader.read (in); Print (in);
        }

        std::cout << std::format ("samples written: {}\nsamples read:    {}\n", samplesWritten, samplesRead) << std::endl;
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
