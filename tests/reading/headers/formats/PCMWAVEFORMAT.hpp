// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_PCMWAVEFORMAT_hpp
#define wavplusplus_tests_PCMWAVEFORMAT_hpp

inline void TestPCMWAVEFORMAT (TestLog & log)
{
    const auto filename           {"test_PCMWAVEFORMAT_header_parsing.wav"};
    constexpr uint32_t sampleRate {44100};
    // Frames should be odd to enable testing for padding for odd channel and byte depth combinations.
    constexpr uint32_t frames     {7};

    struct TestCase
    {
        wav::ExportFormat encoding;
        uint16_t validBitDepth;
    };

    const std::vector<TestCase> testCases
    {
        {wav::ExportFormat::LinearPCM_8, 8},
        {wav::ExportFormat::LinearPCM_16, 16},
        {wav::ExportFormat::LinearPCM_24, 24},
        {wav::ExportFormat::LinearPCM_32, 32},
        {wav::ExportFormat::FloatPCM_32, 32},
        {wav::ExportFormat::FloatPCM_64, 64},

        {wav::ExportFormat::LinearPCM_16, 12}, // 12 in 16
        {wav::ExportFormat::LinearPCM_24, 20}, // 20 in 24
        {wav::ExportFormat::LinearPCM_32, 24}, // 24 in 32
    };

    for (uint16_t channels = 1; channels <= 3; ++channels)
    {
        for (auto const & testCase : testCases)
        {
            const auto [containerSizeInBits, formatCode] = bitDepthAndFormatCode (testCase.encoding);
            const auto validBitDepth {testCase.validBitDepth};

            const bool isLinearPCM {formatCode == wav::FormatCode::LinearPCM};
            const bool isFloatPCM  {formatCode == wav::FormatCode::FloatPCM};

            const auto bytes = buildPCMWAVEFORMAT (formatCode, validBitDepth, containerSizeInBits, channels, sampleRate, frames);
            WriteBytesToFile (filename, bytes);

            std::string failures;

            try
            {
                wav::Reader reader {filename};

                Compare (reader.channels()            == channels,            "unexpected channels",            failures);
                Compare (reader.sampleRate()          == sampleRate,          "unexpected sampleRate",          failures);
                Compare (reader.frames()              == frames,              "unexpected frames",              failures);
                Compare (reader.bitDepth()            == validBitDepth,       "unexpected validBitDepth",       failures);
                Compare (reader.containerSizeInBits() == containerSizeInBits, "unexpected containerSizeInBits", failures);
                Compare (reader.isLinearPCM()         == isLinearPCM,         "unexpected isLinearPCM",         failures);
                Compare (reader.isFloatPCM()          == isFloatPCM,          "unexpected isFloatPCM",          failures);
            }
            catch (wav::ReadError const & e)
            {
                failures = std::string {"wav::ReadError - "} + e.what();
            }

            std::string label {"PCMWAVEFORMAT " + exportFormatShortPrefix (testCase.encoding)};
            if (validBitDepth < containerSizeInBits)
                label += std::format (" ({} bits packed in {})", validBitDepth, containerSizeInBits);
            label += std::format (", {}ch", channels);
            assert (containerSizeInBits % 8 == 0);
            if ((frames * channels * containerSizeInBits / 8U) % 2) label += " (padded)";
            const auto failureSuffix {failures.empty() ? "" : ", failures: " + failures};
            const auto message = std::format ("Read Header - {}{}", label, failureSuffix);

            log.Record (failures.empty(), message);
        }
    }

    std::remove (filename);
}

#endif
