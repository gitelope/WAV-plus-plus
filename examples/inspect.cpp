// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#include <wav-plus-plus/ChunkInfo.hpp>
#include <wav-plus-plus/Writer.hpp>

#include <iostream>

int main ()
{
    // write a file so we can inspect it
    const auto filepath {"stereo.wav"};
    {
        const std::vector<float> samples {0.f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f};

        wav::Writer writer {filepath, wav::ExportFormat::FloatPCM_32, 48000, 2};
        writer.write (samples);
    }

    {
        const wav::ChunkInfo info {filepath};

        // One-line summary:
        // encoding, sample rate, channels, duration, frames, bytes
        std::cout << info.summary() << "\n" << std::endl;

        // In-depth information:
        // List of chunks present (both parsed and skipped)
        // Details for RIFF, fmt, (fact), (PEAK), and data chunks
        std::cout << info << std::endl;
    }

    // Exception handling
    {
        try
        {
            const wav::ChunkInfo info {filepath};
        }
        catch (wav::ReadError const & e)
        {
            // bad path, don't have permission, malformed file, unsupported encoding, etc.

            // handle exception
        }
    }

    return EXIT_SUCCESS;
}
