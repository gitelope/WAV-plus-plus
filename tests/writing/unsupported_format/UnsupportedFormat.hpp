// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_writing_UnsupportedFormat_hpp
#define wavplusplus_tests_writing_UnsupportedFormat_hpp

#include <cstdio>

namespace writing
{

inline void ChannelsExceedsMax (TestLog & log)
{
    constexpr auto channels {static_cast<uint16_t> (wav::maxChannels + 1)};

    const auto filename {"test_writer_channels_exceeds_max.wav"};
    bool passes {false};
    try
    {
        wav::Writer writer {filename, wav::ExportFormat::LinearPCM_16, 44100, channels};
    }
    catch (wav::WriteError const & e)
    {
        passes = (e.code() == wav::WriteErrorCode::ChannelsExceedsMax);
    }

    log.Record (passes, "Writer UnsupportedFormat - constructor - channels exceed maximum imposed by WAV++");
    std::remove (filename);
}

inline void TestUnsupportedFormat (TestLog & log)
{
    ChannelsExceedsMax (log);
}

} // namespace writing

#endif
