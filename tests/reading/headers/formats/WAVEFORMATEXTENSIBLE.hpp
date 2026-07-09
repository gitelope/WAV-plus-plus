// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_WAVEFORMATEXTENSIBLE_hpp
#define wavplusplus_tests_WAVEFORMATEXTENSIBLE_hpp

// Tests all 6 supported encodings,
// packing samples in containers larger than their valid bit depth,
// with 1 to 3 channels,
// and 5 channel masks*.
//
// *The standard does not require that the number of channels equal the number of set
// bits in the channel mask which allows for some freedom when testing.

inline void TestWAVEFORMATEXTENSIBLE (TestLog & log)
{
    const auto filename           {"test_WAVEFORMATEXTENSIBLE_header_parsing.wav"};
    constexpr uint32_t sampleRate {44100};
    // Frames should be odd to create odd data chunk sizes when both byte depth
    // and channels are odd so we can test for the presence of padding.
    constexpr uint32_t frames     {7};

    struct TestCase
    {
        wav::ExportFormat encoding;
        uint16_t validBitDepth;
        uint32_t channelMask;
    };

    const std::vector<TestCase> testCases
    {
        {wav::ExportFormat::LinearPCM_8,   8, wav::SpeakerConfiguration::Unassigned},
        {wav::ExportFormat::LinearPCM_16, 16, wav::SpeakerConfiguration::Stereo},
        {wav::ExportFormat::LinearPCM_24, 24, wav::SpeakerConfiguration::Quadraphonic},
        {wav::ExportFormat::LinearPCM_32, 32, wav::SpeakerConfiguration::Five_Point_One},
        {wav::ExportFormat::FloatPCM_32,  32, wav::SpeakerConfiguration::Seven_Point_One},
        {wav::ExportFormat::FloatPCM_64,  64, wav::SpeakerConfiguration::Unassigned},

        {wav::ExportFormat::LinearPCM_16, 12, wav::SpeakerConfiguration::Stereo},          // 12 bits packed in 16
        {wav::ExportFormat::LinearPCM_24, 20, wav::SpeakerConfiguration::Five_Point_One},  // 20 bits packed in 24
        {wav::ExportFormat::LinearPCM_32, 24, wav::SpeakerConfiguration::Seven_Point_One}, // 24 bits packed in 32
    };

    for (uint16_t channels = 1; channels <= 3; ++channels)
    {
        for (auto const & testCase : testCases)
        {
            const auto [containerSizeInBits, formatCode] = bitDepthAndFormatCode (testCase.encoding);
            const auto subformat {formatCode == wav::FormatCode::LinearPCM ? wav::ExtensibleFormat::LinearPCM : wav::ExtensibleFormat::FloatPCM};

            const auto validBitDepth {testCase.validBitDepth};
            const auto channelMask {testCase.channelMask};

            const bool isLinearPCM {formatCode == wav::FormatCode::LinearPCM};
            const bool isFloatPCM  {formatCode == wav::FormatCode::FloatPCM};

            const auto bytes = buildWAVEFORMATEXTENSIBLE (subformat,
                                                          validBitDepth, containerSizeInBits,
                                                          channels, sampleRate,
                                                          frames, channelMask);
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
                Compare (reader.channelMask()         == channelMask,         "unexpected channelMask",         failures);
            }
            catch (wav::ReadError const & e)
            {
                failures = std::string {"wav::ReadError - "} + e.what();
            }

            std::string label {"WAVEFORMATEXTENSIBLE " + exportFormatShortPrefix (testCase.encoding)};
            if (validBitDepth < containerSizeInBits)
                label += std::format (" ({} bits packed in {})", validBitDepth, containerSizeInBits);
            label += std::format (", {}ch", channels);
            assert (containerSizeInBits % 8 == 0);
            if ((frames * channels * containerSizeInBits / 8U) % 2) label += " (padded)";
            switch (channelMask)
            {
                case wav::SpeakerConfiguration::Unassigned:      label += ", unassigned channels"; break;
                case wav::SpeakerConfiguration::Stereo:          label += ", stereo";              break;
                case wav::SpeakerConfiguration::Quadraphonic:    label += ", quadraphonic";        break;
                case wav::SpeakerConfiguration::Five_Point_One:  label += ", 5.1";                 break;
                case wav::SpeakerConfiguration::Seven_Point_One: label += ", 7.1";                 break;
                default: assert (false);
            }
            const auto failureSuffix {failures.empty() ? "" : ", failures: " + failures};
            const auto message = std::format ("Read Header - {}{}", label, failureSuffix);

            log.Record (failures.empty(), message);
        }
    }

    std::remove (filename);
}

#endif
