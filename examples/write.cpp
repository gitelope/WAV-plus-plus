// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#include <wav-plus-plus/Writer.hpp>

#include <cstdint>
#include <iostream>
#include <memory>
#include <random>

int main ()
{
    const auto     filepath   {"out.wav"};
    const uint32_t sampleRate {48000};
    const auto     encoding   {wav::ExportFormat::FloatPCM_32};
    const uint16_t channels   {2};

    // random number generator
    std::mt19937 generator {std::random_device{}()};
    std::uniform_real_distribution<float> distribution {-1.f, 1.f};
    const auto randomNumber = [&generator, &distribution] {return distribution (generator);};

    // Whole file write with std::vector
    {
        const std::vector<float> buffer {0.f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f};

        wav::Writer writer {filepath, encoding, sampleRate, channels};
        writer.write (buffer);
    }

    // Whole-file write with float[]
    {
        float * buffer = new float[] {0.f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f};
        const auto size {6};

        wav::Writer writer {filepath, encoding, sampleRate, channels};
        writer.write (buffer, size);

        delete[] buffer;
    }

    // Incremental file write with std::vector
    {
        wav::Writer writer {filepath, encoding, sampleRate, channels};

        std::vector<float> buffer (2);

        size_t samplesRemaining {10};

        // write a series of buffers of noise to the file
        while (samplesRemaining)
        {
            for (auto & sample : buffer) sample =  randomNumber();
            samplesRemaining -= writer.write (buffer);
        }
    }

    // Incremental file write with float[]
    {
        wav::Writer writer {filepath, encoding, sampleRate, channels};

        const auto size {2};
        float * buffer = new float[size];

        size_t samplesRemaining {8};

        // write a series of buffers of noise to the file
        while (samplesRemaining)
        {
            for (size_t i=0; i<size; ++i) buffer[i] =  randomNumber();
            samplesRemaining -= writer.write (buffer, size);
        }

        delete[] buffer;
    }

    // Incremental file write with array and writeFrames()
    {
        wav::Writer writer {filepath, encoding, sampleRate, channels};

        const auto sizeInFrames {1};
        float * buffer = new float [sizeInFrames * channels];

        size_t framesRemaining {4};

        // write a series of buffers of noise to the file
        while (framesRemaining)
        {
            for (size_t i=0; i<sizeInFrames * channels; ++i) buffer[i] =  randomNumber();
            framesRemaining -= writer.write (buffer, sizeInFrames);
        }

        delete[] buffer;
    }

    // Exception handling*
    //
    // *This example is somewhat artificial in that you wouldn't proceed from one phase
    // to the next if an exception was actually thrown. Also, you might choose not to
    // handle errors from each of the three phases separately.
    {
        std::unique_ptr<wav::Writer> writer;

        std::vector<float> buffer {0.f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f};

        // 1) Construction
        try
        {
            writer = std::make_unique<wav::Writer> (filepath, encoding, sampleRate, channels);
        }
        catch (wav::WriteError const & e)
        {
            // Failed to create/open file or write the header.

            // handle exception
        }

        // 2) Writing
        try
        {
            writer->write (buffer);
        }
        catch (wav::WriteError const & e)
        {
            // Disk full or unavailable, other I/O failure,
            // WAV file size limit exceeded, API usage error, etc.

            // handle exception
        }

        // 3) Manual Close() allows for exception handling.
        try
        {
            writer->Close();
        }
        catch (wav::WriteError const & e)
        {
            // Failed to flush file buffers;
            // error finalizing data chunk size, RIFF chunk size, or padding;
            // failed to close file; etc.

            // handle exception
        }
    }

    return EXIT_SUCCESS;
}
