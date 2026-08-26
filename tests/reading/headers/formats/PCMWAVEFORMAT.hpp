// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_PCMWAVEFORMAT_hpp
#define wavplusplus_tests_PCMWAVEFORMAT_hpp

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <format>
#include <string>
#include <vector>

inline void TestPCMWAVEFORMAT (TestLog & log)
{
    const auto filename           {"test_PCMWAVEFORMAT_header_parsing.wav"};
    constexpr uint32_t sampleRate {44100};
    // Frames should be odd to enable testing for padding for odd channel and byte depth combinations.
    constexpr uint32_t frames     {7};

    const std::vector<wav::ExportFormat> encodings
    {
        wav::ExportFormat::LinearPCM_8,
        wav::ExportFormat::LinearPCM_16,
        wav::ExportFormat::LinearPCM_24,
        wav::ExportFormat::LinearPCM_32,
        wav::ExportFormat::FloatPCM_32,
        wav::ExportFormat::FloatPCM_64
    };

    for (uint16_t channels = 1; channels <= 3; ++channels)
    {
        for (auto const & encoding : encodings)
        {
            const auto [bitDepth, formatCode] = bitDepthAndFormatCode (encoding);

            const bool isLinearPCM {formatCode == wav::FormatCode::LinearPCM};
            const bool isFloatPCM  {formatCode == wav::FormatCode::FloatPCM};

            const auto bytes = buildPCMWAVEFORMAT (formatCode, bitDepth, bitDepth, channels, sampleRate, frames);
            WriteBytesToFile (filename, bytes);

            std::string failures;

            try
            {
                wav::Reader reader {filename};

                Compare (reader.channels()            == channels,    "unexpected channels",            failures);
                Compare (reader.sampleRate()          == sampleRate,  "unexpected sampleRate",          failures);
                Compare (reader.frames()              == frames,      "unexpected frames",              failures);
                Compare (reader.bitDepth()            == bitDepth,    "unexpected bitDepth",            failures);
                Compare (reader.containerSizeInBits() == bitDepth,    "unexpected containerSizeInBits", failures);
                Compare (reader.isLinearPCM()         == isLinearPCM, "unexpected isLinearPCM",         failures);
                Compare (reader.isFloatPCM()          == isFloatPCM,  "unexpected isFloatPCM",          failures);
            }
            catch (wav::ReadError const & e)
            {
                failures = std::string {"wav::ReadError - "} + e.what();
            }

            std::string label {"PCMWAVEFORMAT " + exportFormatShortPrefix (encoding)};
            label += std::format (", {}ch", channels);
            assert (bitDepth % 8 == 0);
            if ((frames * channels * bitDepth / 8U) % 2) label += " (padded)";
            const auto failureSuffix {failures.empty() ? "" : ", failures: " + failures};
            const auto message = std::format ("Read Header - {}{}", label, failureSuffix);

            log.Record (failures.empty(), message);
        }
    }

    std::remove (filename);
}

#endif
