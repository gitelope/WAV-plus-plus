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

std::vector<uint8_t> buildWAVEFORMATEXTENSIBLE (wav::GUID const &, uint16_t, uint16_t, uint16_t, uint32_t, uint32_t, uint32_t);

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

// Build a WAVEFORMATEXTENSIBLE WAV file (40-byte fmt chunk).
//
// Layout:
//   RIFF header   (12 bytes)
//   fmt  chunk    (8 + 40 = 48 bytes)
//   data chunk    (8 + dataSize bytes)
//   padding byte  (if dataSize is odd)
//
inline std::vector<uint8_t> buildWAVEFORMATEXTENSIBLE (wav::GUID const & subformat,
                                                       uint16_t validBitDepth, uint16_t containerSizeInBits,
                                                       uint16_t channels, uint32_t sampleRate,
                                                       uint32_t frames, uint32_t channelMask)
{
    constexpr uint32_t formatChunkSize {wav::FormatChunkSize::WAVEFORMATEXTENSIBLE};

    assert (containerSizeInBits % 8 == 0);
    const auto blockAlign {static_cast<uint16_t> (channels * containerSizeInBits / 8)};
    const auto byteRate {sampleRate * blockAlign};
    const auto dataChunkSize {frames * blockAlign};

    const uint16_t cbSize {22};

    const auto formatCode {wav::FormatCode::Extensible};


    std::vector<uint8_t> wav;

    // RIFF
    PushRiffHeader (wav, formatChunkSize, dataChunkSize);

    // fmt
    PushFmtHeader               (wav, formatChunkSize);
    Push_WAVEFORMAT_Fmt           (wav, formatCode, channels, sampleRate, byteRate, blockAlign); // 14 bytes
    Push_PCMWAVEFORMAT_Fmt        (wav, containerSizeInBits);                                    //  2 bytes
    Push_WAVEFORMATEX_Fmt         (wav, cbSize);                                                 //  2 bytes
    Push_WAVEFORMATEXTENSIBLE_Fmt (wav, validBitDepth, channelMask, subformat);                  // 22 bytes
                                                                                                 // --------
                                                                                                 // 40 bytes
    // data
    PushSilentDataChunk (wav, dataChunkSize);

    return wav;
}

#endif
