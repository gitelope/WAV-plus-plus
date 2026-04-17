
#ifndef wavplusplus_tests_PCMWAVEFORMAT_hpp
#define wavplusplus_tests_PCMWAVEFORMAT_hpp

std::vector<uint8_t> buildPCMWAVEFORMAT (uint16_t, uint16_t, uint16_t, uint16_t, uint32_t, uint32_t);

inline void TestPCMWAVEFORMAT (TestLog & log)
{
    constexpr std::string filename {"test.wav"};
    constexpr uint32_t sampleRate  {44100};
    constexpr uint32_t frames      {7};

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
            writeFile (filename, bytes);

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

    std::remove (filename.c_str());
}

// Build a PCMWAVEFORMAT WAV file (16-byte fmt chunk).
//
// Layout:
//   RIFF header   (12 bytes)
//   fmt  chunk    (8 + 16 = 24 bytes)
//   data chunk    (8 + dataSize bytes)
//   padding byte  (if dataSize is odd)
//
inline std::vector<uint8_t> buildPCMWAVEFORMAT (uint16_t formatCode, uint16_t validBitDepth, uint16_t containerSizeInBits,
                                                uint16_t channels, uint32_t sampleRate, uint32_t frames)
{
    constexpr uint32_t formatChunkSize {16};

    assert (containerSizeInBits % 8 == 0);
    const auto blockAlign {static_cast<uint16_t> (channels * containerSizeInBits / 8)};
    const auto byteRate {sampleRate * blockAlign};
    const auto dataChunkSize {frames * blockAlign};

    std::vector<uint8_t> wav;

    // RIFF
    push_riff_header (wav, formatChunkSize, dataChunkSize);

    // fmt
    push_fmt_header        (wav, formatChunkSize);
    push_WAVEFORMAT_fmt    (wav, formatCode, channels, sampleRate, byteRate, blockAlign); // 14 bytes
    push_PCMWAVEFORMAT_fmt (wav, validBitDepth);                                          //  2 bytes
                                                                                          // --------
                                                                                          // 16 bytes

    // data
    push_silent_data_chunk (wav, dataChunkSize);

    return wav;
}

#endif
