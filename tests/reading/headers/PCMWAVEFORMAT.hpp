
#ifndef wavplusplus_tests_PCMWAVEFORMAT_hpp
#define wavplusplus_tests_PCMWAVEFORMAT_hpp

std::vector<uint8_t> buildPCMWAVEFORMAT (uint16_t, uint16_t, uint16_t, uint16_t, uint32_t, uint32_t);

inline void TestPCMWAVEFORMAT (TestLog & log)
{
    constexpr std::string filename {"test.wav"};
    constexpr uint32_t sampleRate  {44100};
    constexpr uint32_t frames      {7};

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

            const auto containerSize = bitDepth;
            const auto bytes = buildPCMWAVEFORMAT (formatCode, bitDepth, containerSize, channels, sampleRate, frames);
            writeFile (filename, bytes);

            std::string failures;

            try
            {
                wav::Reader reader {filename};

                Compare (reader.channels()            == channels,    "channels",            failures);
                Compare (reader.sampleRate()          == sampleRate,  "sampleRate",          failures);
                Compare (reader.frames()              == frames,      "frames",              failures);
                Compare (reader.bitDepth()            == bitDepth,    "bitDepth",            failures);
                Compare (reader.containerSizeInBits() == bitDepth,    "containerSizeInBits", failures);
                Compare (reader.isLinearPCM()         == isLinearPCM, "isLinearPCM",         failures);
                Compare (reader.isFloatPCM()          == isFloatPCM,  "isFloatPCM",          failures);
            }
            catch (std::exception const & e)
            {
                failures = std::string {"exception: "} + e.what();
            }

            std::string label {"PCMWAVEFORMAT " + exportFormatShortPrefix (encoding)};
            label += ", " + std::to_string (channels) + "ch";
            assert (bitDepth % 8 == 0);
            const auto blockSizeInBytes {frames * channels * bitDepth / 8U};
            if (blockSizeInBytes % 2) label += " (padded)";
            const std::string message {"Read Header - " + label + (failures.empty() ? "" : " failures: " + failures)};

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
inline std::vector<uint8_t> buildPCMWAVEFORMAT (uint16_t formatCode, uint16_t bitDepth, uint16_t containerSizeInBits,
                                                uint16_t channels, uint32_t sampleRate, uint32_t frames)
{
    constexpr uint32_t formatChunkSize {16};

    assert (bitDepth % 8 == 0);
    const auto bytesPerSample {static_cast<uint16_t> (containerSizeInBits / 8)};
    const auto blockAlign {static_cast<uint16_t> (channels * bytesPerSample)};
    const auto byteRate {sampleRate * blockAlign};
    const auto dataChunkSize {frames * blockAlign};
    const auto riffChunkSize {4 + (8 + formatChunkSize) + (8 + dataChunkSize + (dataChunkSize % 2))};

    std::vector<uint8_t> wav;

    // RIFF
    push_riff_header (wav, riffChunkSize);

    // fmt
    push_fmt_header        (wav, formatChunkSize);
    push_WAVEFORMAT_fmt    (wav, formatCode, channels, sampleRate, byteRate, blockAlign); // 14 bytes
    push_PCMWAVEFORMAT_fmt (wav, bitDepth); // 2 bytes

    // data
    push_silent_data_chunk (wav, dataChunkSize);

    return wav;
}

#endif
