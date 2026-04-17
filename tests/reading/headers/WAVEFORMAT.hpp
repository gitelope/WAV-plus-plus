
#ifndef wavplusplus_tests_WAVEFORMAT_hpp
#define wavplusplus_tests_WAVEFORMAT_hpp

std::vector<uint8_t> buildWAVEFORMAT (uint16_t, uint16_t, uint16_t, uint32_t, uint32_t);

inline void TestWAVEFORMAT (TestLog & log)
{
    const std::string filename    {"test_WAVEFORMAT_header_parsing.wav"};
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

    for (uint16_t channels = 1; channels <= 2; ++channels)
    {
        for (auto const & encoding : encodings)
        {
            const auto [bitDepth, formatCode] = bitDepthAndFormatCode (encoding);
            const bool isLinearPCM {formatCode == wav::FormatCode::LinearPCM};
            const bool isFloatPCM  {formatCode == wav::FormatCode::FloatPCM};

            const auto bytes = buildWAVEFORMAT (formatCode, bitDepth, channels, sampleRate, frames);
            writeFile (filename, bytes);

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

            std::string label {"WAVEFORMAT " + exportFormatShortPrefix (encoding)};
            label += std::format (", {}ch", channels);
            assert (bitDepth % 8 == 0);
            if ((frames * channels * bitDepth / 8U) % 2) label += " (padded)";
            const auto failureSuffix {failures.empty() ? "" : ", failures: " + failures};
            const auto message = std::format ("Read Header - {}{}", label, failureSuffix);

            log.Record (failures.empty(), message);
        }
    }

    std::remove (filename.c_str());
}

// Build a WAVEFORMAT WAV file (14-byte fmt chunk).
//
// Layout:
//   RIFF header   (12 bytes)
//   fmt  chunk    (8 + 14 = 22 bytes)
//   data chunk    (8 + dataSize bytes)
//   padding byte  (if dataSize is odd)
//
inline std::vector<uint8_t> buildWAVEFORMAT (uint16_t formatCode, uint16_t bitDepth, uint16_t channels,
                                             uint32_t sampleRate, uint32_t frames)
{
    constexpr uint32_t formatChunkSize {14};

    assert (bitDepth % 8 == 0);
    const auto blockAlign {static_cast<uint16_t> (channels * bitDepth / 8)};
    const auto byteRate {sampleRate * blockAlign};
    const auto dataChunkSize {frames * blockAlign};

    std::vector<uint8_t> wav;

    // RIFF
    push_riff_header (wav, formatChunkSize, dataChunkSize);

    // fmt
    push_fmt_header        (wav, formatChunkSize);
    push_WAVEFORMAT_fmt    (wav, formatCode, channels, sampleRate, byteRate, blockAlign); // 14 bytes

    // data
    push_silent_data_chunk (wav, dataChunkSize);

    return wav;
}

#endif
