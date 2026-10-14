// WAV++ (c) 2025 David Adams
// SPDX-License-Identifier: MIT

#include <wav-plus-plus/Reader.hpp>
#include <wav-plus-plus/Writer.hpp>

#include <iostream>
#include <fstream>
#include <memory>

int main ()
{
    std::vector<float> out {0.f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f};

    // Write a stereo WAV that will be used by most examples.
    const auto filepath {"stereo.wav"};
    {
        wav::Writer writer {filepath, wav::ExportFormat::FloatPCM_32, 48000, 2};
        writer.write (out);
    }
    // Write a 3-channel WAV.
    const auto filepath_3_channel {"3-channel.wav"};
    {
        wav::Writer writer {filepath_3_channel, wav::ExportFormat::FloatPCM_32, 48000, 3};
        writer.write (out);
    }

    // Get file info from Reader.
    {
        wav::Reader reader {filepath};

        const auto channels   {reader.channels()};
        const auto frames     {reader.frames()};
        const auto samples    {reader.samples()};     // = frames * channels
        const auto sampleRate {reader.sampleRate()};

        const auto bitDepth            {reader.bitDepth()};
        const auto containerSizeInBits {reader.containerSizeInBits()};
        const auto isLinearPCM         {reader.isLinearPCM()};
        const auto isFloatPCM          {reader.isFloatPCM()};
    }

    // Get file info from FileInfo struct.
    {
        wav::Reader reader {filepath};

        const auto fileInfo            {reader.getInfo()};

        const auto channels            {fileInfo.channels};
        const auto frames              {fileInfo.frames};
        const auto samples             {channels * frames};
        const auto sampleRate          {fileInfo.sampleRate};

        const auto bitDepth            {fileInfo.bitDepth};
        const auto containerSizeInBits {fileInfo.containerSizeInBits};
        const auto isLinearPCM         {fileInfo.isLinearPCM};
        const auto isFloatPCM          {fileInfo.isFloatPCM};
    }

    // Short formatted one-line summary of file attributes
    {
        wav::Reader reader {filepath};
        std::cout << reader.getInfo() << std::endl;
    }

    ///////////// INTERLEAVED READING /////////////

    // Whole-file read into std::vector
    {
        wav::Reader reader {filepath};
        std::vector<float> buffer (reader.samples());
        const auto samplesRead = reader.read (buffer);
    }
    // Whole-file read into float[]
    {
        wav::Reader reader {filepath};
        const auto size {reader.samples()};
        float * buffer = new float [size];
        const auto samplesRead = reader.read (buffer, size);
        delete[] buffer;
    }

    // Chunked read into std::vector
    {
        wav::Reader reader {filepath};

        const auto bufferSize {2};
        std::vector<float> buffer (bufferSize);

        size_t samplesRemaining = reader.samples();
        while (samplesRemaining > 0)
        {
            const auto samplesRead = reader.read (buffer);
            samplesRemaining -= samplesRead;
            // process (samples)
        }
    }
    // Chunked read into float[]
    {
        wav::Reader reader {filepath};

        const auto bufferSize {2};
        float * buffer = new float [bufferSize];

        size_t samplesRemaining {reader.samples()};
        while (samplesRemaining > 0)
        {
            const auto samplesRead = reader.read (buffer, bufferSize);
            samplesRemaining -= samplesRead;
            // process (samples)
        }

        delete[] buffer;
    }

    // readFrames: Chunked read into std::vector
    {
        wav::Reader reader {filepath};

        const auto bufferSizeInFrames {2};
        std::vector<float> buffer (bufferSizeInFrames * reader.channels());

        size_t framesRemaining = reader.frames();
        while (framesRemaining > 0)
        {
            const auto framesRead = reader.readFrames (buffer);
            framesRemaining -= framesRead;
            // process (samples)
        }
    }

    ///////////// DEINTERLEAVED READING /////////////

    // Reading a fixed number of channels into named vectors
    {
        wav::Reader reader {filepath};
        const auto frames {reader.frames()};

        std::vector<float> left  (frames);
        std::vector<float> right (frames);

        std::vector<float *> destinations {left.data(), right.data()};

        const auto framesRead = reader.readFrames (destinations.data(), destinations.size(), frames);
    }

    // Reading a fixed number of channels into named arrays
    {
        wav::Reader reader {filepath_3_channel};
        const auto frames {reader.frames()};

        float * left   = new float [frames];
        float * right  = new float [frames];
        float * center = new float [frames];

        float * destinations[] = {left, right, center};

        const auto framesRead = reader.readFrames (destinations, 3, frames);

        delete[] left;
        delete[] right;
        delete[] center;
    }

    // Reading all channels into a vector of vectors
    {
        wav::Reader reader {filepath};

        const auto channels {reader.channels()};
        const auto frames   {reader.frames()};

        std::vector<std::vector<float>> buffers (channels);
        for (auto & buffer : buffers)
        {
            buffer.resize (frames);
        }

        std::vector<float*> destinations;
        destinations.reserve (channels);
        for (auto & channel : buffers)
        {
            destinations.push_back (channel.data());
        }

        const auto framesRead = reader.readFrames (destinations.data(), channels, frames);
    }

    // Reading all channels into a single vector
    {
        wav::Reader reader {filepath};

        const auto channels {reader.channels()};
        const auto frames   {reader.frames()};
        const auto samples  {reader.samples()};

        std::vector<float> buffer (samples);

        std::vector<float *> destinations (channels);
        for (size_t i = 0; i < channels; ++i)
        {
            destinations[i] = buffer.data() + frames * i;
        }

        const auto framesRead = reader.readFrames (destinations.data(), channels, frames);
    }

    // Reading all channels into a single array
    {
        wav::Reader reader {filepath};

        const auto channels {reader.channels()};
        const auto frames   {reader.frames()};
        const auto samples  {reader.samples()};

        float * buffer = new float [samples];

        float * * destinations = new float * [channels];
        for (size_t i = 0; i < channels; ++i)
        {
            destinations[i] = buffer + frames * i;
        }

        const auto framesRead = reader.readFrames (destinations, channels, frames);

        delete[] destinations;
        delete[] buffer;
    }

    // Exception handling*
    //
    // *This example is somewhat artificial in that you wouldn't proceed to reading the file
    // if there was an exception thrown while opening it and parsing the header.
    // Additionally, you might choose not to handle errors from the two phases separately.
    {
        std::unique_ptr<wav::Reader> reader;

        // 1) Construction
        try
        {
            reader = std::make_unique<wav::Reader> (filepath);
        }
        catch (wav::ReadError const & e)
        {
            // File not found, not a valid WAV file,
            // unsupported encoding, etc.

            // handle exception
        }

        // 2) Reading
        try
        {
            std::vector<float> buffer (reader->samples());
            reader->read (buffer);
        }
        catch (wav::ReadError const & e)
        {
            // I/O failure, API usage errors, etc.

            // handle exception
        }
    }
}
