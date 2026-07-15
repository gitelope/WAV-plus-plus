// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_reading_InvalidArgument_hpp
#define wavplusplus_tests_reading_InvalidArgument_hpp

#include <cstdio>
#include <vector>

inline void SamplesRequestedExceedsDestinationSize (TestLog & log)
{
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 24, 24, 1, 96000, 8);
    const auto filename {"test_samples_requested_exceeds_destination.wav"};
    WriteBytesToFile (filename, bytes);

    bool passes {false};
    try
    {
        wav::Reader reader {filename};
        std::vector<float> destination (1);
        reader.read (destination, 2);
    }
    catch (wav::ReadError const & e)
    {
        passes = (e.code() == wav::ReadErrorCode::SamplesRequestedExceedsDestinationSize);
    }

    log.Record (passes, "Reader InvalidArgument - read (vector &, size_t) - samples requested exceeds destination size");
    std::remove (filename);
}

inline void DestinationFrameSizeIsFractional (TestLog & log)
{
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 16, 16, 2, 44100, 8);
    const auto filename {"test_destination_not_whole_frames.wav"};
    WriteBytesToFile (filename, bytes);

    bool passes {false};
    try
    {
        wav::Reader reader {filename};
        std::vector<float> destination (3);
        reader.readFrames (destination);
    }
    catch (wav::ReadError const & e)
    {
        passes = (e.code() == wav::ReadErrorCode::DestinationFrameSizeIsFractional);
    }

    log.Record (passes, "Reader InvalidArgument - readFrames (vector &) - destination size in frames is not a whole number");
    std::remove (filename);
}

inline void FramesRequestedExceedsDestinationSize (TestLog & log)
{
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 16, 16, 2, 44100, 8);
    const auto filename {"test_frames_requested_exceeds_destination.wav"};
    WriteBytesToFile (filename, bytes);

    bool passes {false};
    try
    {
        wav::Reader reader {filename};
        std::vector<float> destination (4);
        reader.readFrames (destination, 3);
    }
    catch (wav::ReadError const & e)
    {
        passes = (e.code() == wav::ReadErrorCode::FramesRequestedExceedsDestinationSize);
    }

    log.Record (passes, "Reader InvalidArgument - readFrames (vector &, size_t) - frames requested exceeds destination size");
    std::remove (filename);
}

inline void ChannelsRequestedExceedsChannelCount (TestLog & log)
{
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 16, 16, 1, 44100, 8);
    const auto filename {"test_channels_requested_exceeds_file.wav"};
    WriteBytesToFile (filename, bytes);

    bool passes {false};
    try
    {
        wav::Reader reader {filename};
        float left {};
        float right {};
        float * destinations[] {&left, &right};
        reader.read (destinations, 2, 1);
    }
    catch (wav::ReadError const & e)
    {
        passes = (e.code() == wav::ReadErrorCode::ChannelsRequestedExceedsChannelCount);
    }

    log.Record (passes, "Reader InvalidArgument - read (T ** const, size_t, size_t) - channels requested exceeds channel count");
    std::remove (filename);
}

inline void TestInvalidArgument (TestLog & log)
{
    SamplesRequestedExceedsDestinationSize (log);
    DestinationFrameSizeIsFractional (log);
    FramesRequestedExceedsDestinationSize (log);
    ChannelsRequestedExceedsChannelCount (log);
}

#endif
