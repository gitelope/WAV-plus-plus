
#include <wav-plus-plus/Reader.hpp>
#include <wav-plus-plus/Writer.hpp>

#include <array>
#include <cassert>
#include <format>
#include <iostream>
#include <fstream>
#include <vector>

//template <typename T> void Print (std::vector<T> const &);
template <typename T> void Print (std::vector<T> const &, size_t);
template <typename T> void Print (T const * const, size_t);

template <typename T>
std::ostream & operator<< (std::ostream & os, std::vector<T> const & testSequence);

//template <typename T> void Print (std::vector<std::vector<T>> const &);
//template <typename T> void Print (T const * const * const array, size_t, size_t);

void Section (std::string const & title);
void Example (std::string const & title);

int main ()
{
    const std::string filename {"test.wav"};

    Section ("interleaved");

    // write test file (and display file info)
    {
        std::vector<float> out = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
        std::cout << "\nwriting: [" << out << "]" << std::endl;

        wav::Writer writer {filename, wav::ExportFormat::LinearPCM_24, 48000, 2};
        writer.write (out);
    }
    {
        wav::Reader reader {filename};
        std::cout << reader.getInfo() << std::endl;
    }

    Example ("read into std::vector");
    {
        wav::Reader reader {filename};
        std::vector<float> in (reader.samples());
        const auto samplesRead = reader.read (in);

        Print (in, samplesRead);
    }

    Example ("read into C array");
    {
        wav::Reader reader {filename};
        const size_t size {reader.samples()};
        float * const in = new float [size];
        const auto samplesRead = reader.read (in, size);

        Print (in, samplesRead);

        delete[] in;
    }

    Section ("de-interleaved");

    // write test file (and display file info)
    {
        const std::vector<float> out {0.1, 1.1, 0.2, 1.2, 0.3, 1.3, 0.4, 1.4};
        std::cout << "\nwriting: [" << out << "]" << std::endl;

        wav::Writer writer {filename, wav::ExportFormat::FloatPCM_32, 48000, 2};
        writer.write (out);
    }
    {
        wav::Reader reader {filename};
        std::cout << reader.getInfo() << std::endl;
    }

    Example ("stereo read into separate std::vectors");
    {
        wav::Reader reader {filename};
        const size_t frames {reader.frames()};

        std::vector<float> left (frames);
        std::vector<float> right (frames);
        float * destinations[] = {left.data(), right.data()};

        const auto framesRead = reader.read (destinations, 2, frames);

        Print (left, framesRead);
        Print (right, framesRead);
    }

    Example ("stereo read into separate C arrays");
    {
        wav::Reader reader {filename};
        const size_t frames {reader.frames()};

        float * left = new float [frames];
        float * right = new float [frames];
        float * destinations[] = {left, right};

        const auto framesRead = reader.read (destinations, 2, frames);

        Print (left, framesRead);
        Print (right, framesRead);

        delete[] left;
        delete[] right;
    }

    Example ("stereo read into one std::vector");
    {
        wav::Reader reader {filename};
        const size_t frames {reader.frames()};
        constexpr size_t channels {2};

        std::vector<float> deinterleaved (channels * frames);
        float * destinations[] = {deinterleaved.data(), deinterleaved.data() + frames};

        const auto framesRead = reader.read (destinations, channels, frames);

        Print (deinterleaved, channels * framesRead);
    }

    Example ("stereo read into one C array");
    {
        wav::Reader reader {filename};
        const size_t frames {reader.frames()};
        constexpr size_t channels {2};

        float * deinterleaved = new float [channels * frames];

        float * left = deinterleaved;
        float * right = deinterleaved + frames;
        float * destinations[] = {left, right};

        const auto framesRead = reader.read (destinations, channels, frames);

        Print (deinterleaved, channels * framesRead);

        delete[] deinterleaved;
    }

    // write test file (and display file info)
    {
        const std::vector<float> out {0.1, 1.1, 2.1, 0.2, 1.2, 2.2, 0.3, 1.3, 2.3, 0.4, 1.4, 2.4};
        std::cout << "\nwriting: [" << out << "]" << std::endl;

        wav::Writer writer {filename, wav::ExportFormat::FloatPCM_32, 48000, 3};
        writer.write (out);
    }
    {
        wav::Reader reader {filename};
        std::cout << reader.getInfo() << std::endl;
    }

    Example ("read an arbitrary number of channels into one std::vector");
    {
        wav::Reader reader {filename};
        const size_t frames {reader.frames()};
        const size_t channels {reader.channels()};

        std::vector<float> deinterleaved (channels * frames);

        std::vector<float *> destinations (channels);
        for (size_t i = 0; i < channels; ++i)
        {
            destinations[i] = deinterleaved.data() + frames * i;
        }

        const auto framesRead = reader.read (destinations.data(), channels, frames);

        Print (deinterleaved, channels * framesRead);
    }

    Example ("read an arbitrary number of channels into one C array");
    {
        wav::Reader reader {filename};
        const size_t frames {reader.frames()};
        const size_t channels {reader.channels()};

        float * deinterleaved = new float [channels * frames];

        float * * destinations = new float * [channels];
        for (size_t i = 0; i < channels; ++i)
        {
            destinations[i] = deinterleaved + frames * i;
        }

        const auto framesRead = reader.read (destinations, channels, frames);

        Print (deinterleaved, channels * framesRead);

        delete[] destinations;
        delete[] deinterleaved;
    }

    Section ("incremental");

    std::cout << "\nincremental writing: " << std::endl;
    {
        size_t samplesWritten {0};

        std::vector<float> buffer1 {1.f, 2.f, 3.f, 4.f};
        std::vector<float> buffer2 {5.f, 6.f, 7.f, 8.f};
        Print (buffer1, buffer1.size());
        Print (buffer2, buffer2.size());

        wav::Writer writer {filename, wav::ExportFormat::FloatPCM_32, 48000, 1};

        samplesWritten += writer.write (buffer1);
        samplesWritten += writer.write (buffer2);

        std::cout << "\n" << samplesWritten << " samples written." << std::endl;
    }
    {
        wav::Reader reader {filename};
        std::cout << "\n" << reader.getInfo() << std::endl;
    }

    constexpr size_t readBufferSize {3};

    Example ("incremental read with std::vector (samples requested = container size)");
    {
        wav::Reader reader {filename};
        std::vector<float> in (readBufferSize);

        // 2 buffer-filling reads and 1 partially-buffer read
        size_t samplesRead {0};
        while (samplesRead < reader.samples())
        {
            const size_t n = reader.read (in);
            samplesRead += n;
            Print (in, n);
        }
    }
    Example ("incremental read with std::array (samples requested = container size)");
    {
        wav::Reader reader {filename};
        std::array<float,readBufferSize> in;

        // 2 buffer-filling reads and 1 partially-buffer read
        size_t samplesRead {0};
        while (samplesRead < reader.samples())
        {
            const size_t n = reader.read (in);
            samplesRead += n;
            Print (in.data(), n);
        }
    }

    Example ("incremental read with stack-allocated C array (samples requested = container size)");
    {
        wav::Reader reader {filename};
        float in [readBufferSize];

        // 2 buffer-filling reads and 1 partially-buffer read
        size_t samplesRead {0};
        while (samplesRead < reader.samples())
        {
            const size_t n = reader.read (in);
            samplesRead += n;
            Print (in, n);
        }
    }

    Section ("error handling");

    Example ("requested file doesn't exist");
    {
        try
        {
            wav::Reader reader {"nonexistent.wav"};
        }
        catch (wav::ReadError const & e)
        {
            std::cerr << "wav::ReadError: " << '\"' << e.what() << '\"' << std::endl;
        }
    }

    Example ("invalid WAV file\n(File is empty so RIFF chunk ID cannot be found.)");
    {
        const std::string filename {"cest_nest_pas_un_wav.wav"};
        {
            std::ofstream out {filename, std::ios::binary};
        }
        try
        {
            wav::Reader reader {filename};
        }
        catch (wav::ReadError const & e)
        {
            std::cerr << "wav::ReadError: " << '\"' << e.what() << '\"' << std::endl;
        }
    }

    Example ("channel count mismatch during read\n(It's ok to request less than the total but not ok to request more than are present.)");
    {
        {
            std::cout << "Writing 2 channels of audio." << std::endl;
            const std::vector<float> out {0.1f, 0.2f, 0.3f, 0.4f};
            wav::Writer writer {filename, wav::ExportFormat::FloatPCM_32, 48000, 2};
            writer.write (out);
        }
        try
        {
            wav::Reader reader {filename};
            const size_t frames {reader.frames()};

            const size_t channels {3}; // Oops!
            std::cout << "Requesting " << channels << " channels of audio." << std::endl;

            std::vector<float> deinterleaved (frames * channels);
            std::vector<float *> destinations (channels);
            for (size_t i=0; i < channels; ++i)
            {
                destinations[i] = deinterleaved.data() + i * frames;
            }
            reader.read (destinations.data(), channels, frames);
        }
        catch (wav::ReadError const & e)
        {
            std::cout << "wav::ReadError: " << e.what() << std::endl;
        }
    }

    return 0;
}

template <typename T>
void Print (std::vector<T> const & interleaved, size_t elementsToPrint)
{
    for (size_t i=0; i < elementsToPrint; ++i)
    {
        std::cout << std::format ("{}", interleaved[i]);
        if (i < elementsToPrint - 1) std::cout << ", ";
    }
    std::cout << std::endl;
}

template <typename T>
void Print (T const * const array, size_t elementsToPrint)
{
    for (size_t i = 0; i < elementsToPrint; ++i)
    {
        std::cout << std::format ("{}", array[i]);
        if (i < elementsToPrint - 1) std::cout << ", ";
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

template <typename T>
std::ostream & operator<< (std::ostream & os, std::vector<T> const & testSequence)
{
    for (size_t i = 0; i < testSequence.size(); ++i)
    {
        os << std::format ("{}", testSequence[i]);
        if (i < testSequence.size() - 1) os << ", ";
    }
    return os;
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

 /*
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
*/
