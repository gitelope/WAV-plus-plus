
#include <wav-plus-plus/Reader.hpp>
#include <wav-plus-plus/Writer.hpp>

#include <vector>
#include <iostream>
#include <format>
#include <cassert>
#include <array>

template <typename T> void Print (std::vector<T> const &);
template <typename T> void Print (std::vector<std::vector<T>> const &);

template <typename T> void Print (T const * const, size_t);
template <typename T> void Print (T const * const * const array, size_t, size_t);

void Section (std::string const & title);
void Example (std::string const & title);

int main ()
{
    const auto filename {"test.wav"};
    {
        // write a WAV file
        {
            const std::vector<float> out {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
            wav::Writer writer {filename, wav::ExportFormat::LinearPCM_24, 48000, 2};
            writer.write (out);
        }

        Section ("formatted file info");
        {
            wav::Reader reader {filename};
            std::cout << "\n" << reader.getInfo() << std::endl;
        }

        Section ("interleaved");

        Example ("read into std::vector");
        {
            wav::Reader reader {filename};
            std::vector<float> in (reader.samples());
            reader.read (in);
            Print (in);
        }

        Example ("read into C array");
        {
            wav::Reader reader {filename};
            const size_t size {reader.samples()};
            float * const in = new float [size];
            reader.read (in, size);
            Print (in, size);

            delete[] in;
        }
    }
    {
        Section ("de-interleaved");
        {
            {
                const std::vector<float> out {0.1, 1.1, 0.2, 1.2, 0.3, 1.3, 0.4, 1.4};

                wav::Writer writer {filename, wav::ExportFormat::FloatPCM_32, 48000, 2};
                writer.write (out);
            }

            Example ("stereo read into separate std::vectors");
            {
                wav::Reader reader {filename};
                const size_t frames {reader.frames()};

                std::vector<float> left (frames);
                std::vector<float> right (frames);
                float * channelPointers[] = {left.data(), right.data()};

                reader.read (channelPointers, 2, frames);
                Print (left);
                Print (right);
            }

            Example ("stereo read into separate C arrays");
            {
                wav::Reader reader {filename};
                const size_t frames {reader.frames()};

                float * left = new float [frames];
                float * right = new float [frames];
                float * channelPointers[] = {left, right};

                reader.read (channelPointers, 2, frames);
                Print (left, frames);
                Print (right, frames);

                delete[] left;
                delete[] right;
            }

            Example ("stereo read into one std::vector");
            {
                wav::Reader reader {filename};
                const size_t frames {reader.frames()};

                std::vector<float> deinterleaved (2 * frames);
                float * channelPointers[] = {deinterleaved.data(), deinterleaved.data() + frames};

                reader.read (channelPointers, 2, frames);
                Print (deinterleaved);
            }

            Example ("stereo read into one C array");
            {
                wav::Reader reader {filename};
                const size_t frames {reader.frames()};

                float * deinterleaved = new float [2 * frames];

                float * left = deinterleaved;
                float * right = deinterleaved + frames;
                float * channelPointers[] = {left, right};

                reader.read (channelPointers, 2, frames);
                Print (deinterleaved, 2 * frames);

                delete[] deinterleaved;
            }
        }

        // arbitrary number of channels into contiguous storage:
        {
            {
                const std::vector<float> out {0.1, 1.1, 2.1, 0.2, 1.2, 2.2, 0.3, 1.3, 2.3, 0.4, 1.4, 2.4};

                wav::Writer writer {filename, wav::ExportFormat::FloatPCM_32, 48000, 3};
                writer.write (out);
            }

            Example ("read an arbitrary number of channels into one std::vector");
            {
                wav::Reader reader {filename};
                const size_t frames {reader.frames()};
                const size_t channels {reader.channels()};

                std::vector<float> deinterleaved (channels * frames);

                std::vector<float *> channelPointers (channels);
                for (size_t i = 0; i < channels; ++i)
                {
                    channelPointers[i] = deinterleaved.data() + frames * i;
                }

                reader.read (channelPointers.data(), channels, frames);

                Print (deinterleaved);
            }

            Example ("read of an arbitrary number of channels into one C array");
            {
                wav::Reader reader {filename};
                const size_t frames {reader.frames()};
                const size_t channels {reader.channels()};

                float * deinterleaved = new float [channels * frames];

                float * * channelPointers = new float * [channels];
                for (size_t i = 0; i < channels; ++i)
                {
                    channelPointers[i] = deinterleaved + frames * i;
                }

                reader.read (channelPointers, channels, frames);

                Print (deinterleaved, channels * frames);

                delete[] channelPointers;
                delete[] deinterleaved;
            }
        }
    }

    Section ("incremental");
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

/*
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
 */

/*
template <typename T>
void Print (T const * const * const array, size_t channels, size_t frames)
{
    for (size_t c = 0; c < channels; ++c)
    {
        for (size_t n = 0; n < frames; ++n)
        {
            std::cout << std::format ("{}, ", array[c][n]);
        }
        std::cout << '\n';
    }
    std::cout << std::endl;
}
 */

template <typename T>
void Print (T const * const array, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        std::cout << std::format ("{}", array[i]);
        if (i < size - 1) std::cout << ", ";
    }
    std::cout << std::endl;
}

void Section (std::string const & title)
{
    const std::string line (title.size() + 2, '-');
    std::cout << '\n' << line << '\n' << '|' << title << '|' << '\n' << line << std::endl;
}

void Example (std::string const & title)
{
    std::cout << '\n' << title << std::endl;
}
