
#include <wav-plus-plus/Writer.hpp>
#include <iostream>

int main ()
{
    const auto filepath   {"out.wav"};
    const auto sampleRate {48000};
    const auto encoding   {wav::ExportFormat::FloatPCM_32};
    const auto channels   {2};

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

        auto samplesRemaining {10};

        // write a series of buffers of noise to the file
        while (samplesRemaining)
        {
            for (auto & sample : buffer) sample =  (2.f * rand() / RAND_MAX) - 1.f;
            samplesRemaining -= writer.write (buffer);
        }
    }

    // Incremental file write with float[]
    {
        wav::Writer writer {filepath, encoding, sampleRate, channels};

        const auto size {2};
        float * buffer = new float[size];

        auto samplesRemaining {8};

        // write a series of buffers of noise to the file
        while (samplesRemaining)
        {
            for (size_t i=0; i<size; ++i) buffer[i] =  (2.f * rand() / RAND_MAX) - 1.f;
            samplesRemaining -= writer.write (buffer, size);
        }

        delete[] buffer;
    }

    // Incremental file write with array and writeFrames()
    {
        wav::Writer writer {filepath, encoding, sampleRate, channels};

        const auto sizeInFrames {1};
        float * buffer = new float [sizeInFrames * channels];

        auto framesRemaining {4};

        // write a series of buffers of noise to the file
        while (framesRemaining)
        {
            for (size_t i=0; i<sizeInFrames * channels; ++i) buffer[i] =  (2.f * rand() / RAND_MAX) - 1.f;
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
