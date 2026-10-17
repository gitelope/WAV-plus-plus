// WAV++
// Copyright (c) 2025 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_writing_InvalidArgument_hpp
#define wavplusplus_tests_writing_InvalidArgument_hpp

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

inline void SampleRateZero (TestLog & log)
{
    constexpr std::uint32_t sampleRate {0};

    const auto filename {"test_writer_sample_rate_zero.wav"};
    bool passes {false};
    try
    {
        wav::Writer writer {filename, wav::ExportFormat::LinearPCM_16, sampleRate, 1};
    }
    catch (wav::WriteError const & e)
    {
        passes = (e.code() == wav::WriteErrorCode::SampleRateZero);
    }

    log.Record (passes, "Writer InvalidArgument - constructor - sample rate zero");
    std::remove (filename);
}

inline void ChannelsZero (TestLog & log)
{
    constexpr std::uint16_t channels {0};

    const auto filename {"test_writer_channels_zero.wav"};
    bool passes {false};
    try
    {
        wav::Writer writer {filename, wav::ExportFormat::LinearPCM_16, 44100, channels};
    }
    catch (wav::WriteError const & e)
    {
        passes = (e.code() == wav::WriteErrorCode::ChannelsZero);
    }

    log.Record (passes, "Writer InvalidArgument - constructor - channels zero");
    std::remove (filename);
}

inline void BufferSizeZero (TestLog & log)
{
    constexpr std::size_t bufferSize {0};

    const auto filename {"test_writer_buffer_size_zero.wav"};
    bool passes {false};
    try
    {
        wav::Writer writer {filename, wav::ExportFormat::LinearPCM_16, 44100, 1, 0, bufferSize};
    }
    catch (wav::WriteError const & e)
    {
        passes = (e.code() == wav::WriteErrorCode::BufferSizeZero);
    }

    log.Record (passes, "Writer InvalidArgument - constructor - buffer size zero");
    std::remove (filename);
}

inline void ChannelMaskInvalid (TestLog & log)
{
    constexpr std::uint32_t invalidMask {0xFFFFFFFF};

    const auto filename {"test_writer_channel_mask_invalid.wav"};
    bool passes {false};
    try
    {
        wav::Writer writer {filename, wav::ExportFormat::LinearPCM_16, 44100, 1, invalidMask};
    }
    catch (wav::WriteError const & e)
    {
        passes = (e.code() == wav::WriteErrorCode::ChannelMaskInvalid);
    }

    log.Record (passes, "Writer InvalidArgument - constructor - channel mask invalid");
    std::remove (filename);
}

inline void SamplesToWriteExceedsSourceSize (TestLog & log)
{
    std::size_t sourceSize {1};
    std::size_t samplesToWrite {sourceSize + 1};

    const auto filename {"test_writer_samples_exceeds_source.wav"};
    bool passes {false};
    try
    {
        wav::Writer writer {filename, wav::ExportFormat::LinearPCM_16, 44100, 1};
        std::vector<float> source (sourceSize, 0.f);
        writer.write (source, samplesToWrite);
    }
    catch (wav::WriteError const & e)
    {
        passes = (e.code() == wav::WriteErrorCode::SamplesToWriteExceedsSourceSize);
    }

    log.Record (passes, "Writer InvalidArgument - write (vector &, size_t) - samples to write exceeds source size");
    std::remove (filename);
}

inline void SourceFrameSizeIsFractional (TestLog & log)
{
    constexpr std::uint16_t channels {2};
    constexpr std::size_t sourceSizeInSamples {3}; // 3/2 = 1.5 frames

    const auto filename {"test_writer_source_not_whole_frames.wav"};
    bool passes {false};
    try
    {
        wav::Writer writer {filename, wav::ExportFormat::LinearPCM_16, 44100, channels};
        std::vector<float> source (sourceSizeInSamples, 0.f);
        writer.writeFrames (source);
    }
    catch (wav::WriteError const & e)
    {
        passes = (e.code() == wav::WriteErrorCode::SourceFrameSizeIsFractional);
    }

    log.Record (passes, "Writer InvalidArgument - writeFrames (vector &) - source size is not a whole number of frames");
    std::remove (filename);
}

inline void FramesToWriteExceedsSourceSize (TestLog & log)
{
    constexpr std::size_t sourceSizeInFrames {2};
    constexpr std::size_t framesToWrite {sourceSizeInFrames + 1};

    constexpr std::uint16_t channels {2};

    const auto filename {"test_writer_frames_exceeds_source.wav"};
    bool passes {false};
    try
    {
        wav::Writer writer {filename, wav::ExportFormat::LinearPCM_16, 44100, channels};
        std::vector<float> source (channels * sourceSizeInFrames, 0.f);
        writer.writeFrames (source, framesToWrite);
    }
    catch (wav::WriteError const & e)
    {
        passes = (e.code() == wav::WriteErrorCode::FramesToWriteExceedsSourceSize);
    }

    log.Record (passes, "Writer InvalidArgument - writeFrames (vector &, size_t) - frames to write exceeds source size");
    std::remove (filename);
}

inline void TestInvalidArgumentForWriting (TestLog & log)
{
    SampleRateZero (log);
    ChannelsZero (log);
    BufferSizeZero (log);
    ChannelMaskInvalid (log);
    SamplesToWriteExceedsSourceSize (log);
    SourceFrameSizeIsFractional (log);
    FramesToWriteExceedsSourceSize (log);
}

#endif
