
#ifndef wavplusplus_tests_PCMWAVEFORMAT_hpp
#define wavplusplus_tests_PCMWAVEFORMAT_hpp

std::vector<uint8_t> buildPCMWAVEFORMAT (uint16_t, uint16_t, uint16_t, uint32_t, uint32_t);

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

            const auto bytes = buildPCMWAVEFORMAT (formatCode, bitDepth, channels, sampleRate, frames);
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
inline std::vector<uint8_t> buildPCMWAVEFORMAT (uint16_t formatCode, uint16_t bitDepth, uint16_t channels,
                                                uint32_t sampleRate, uint32_t frames)
{
    constexpr uint32_t formatChunkSize {16};

    assert (bitDepth % 8 == 0);
    const auto bytesPerSample {static_cast<uint16_t> (bitDepth / 8)};
    const auto blockAlign {static_cast<uint16_t> (channels * bytesPerSample)};
    const auto byteRate {sampleRate * blockAlign};
    const auto dataChunkSize {frames * blockAlign};
    const auto riffChunkSize {4 + (8 + formatChunkSize) + (8 + dataChunkSize + (dataChunkSize % 2))};

    std::vector<uint8_t> wav;

    // RIFF header
    push_fourCC    (wav, "RIFF");
    push_uint32_le (wav, riffChunkSize);
    push_fourCC    (wav, "WAVE");

    // fmt chunk (16 bytes)
    // WAVEFORMAT
    push_fourCC    (wav, "fmt ");
    push_uint32_le (wav, formatChunkSize);
    push_uint16_le (wav, formatCode);
    push_uint16_le (wav, channels);
    push_uint32_le (wav, sampleRate);
    push_uint32_le (wav, byteRate);
    push_uint16_le (wav, blockAlign);
    // PCMWAVEFORMAT
    push_uint16_le (wav, bitDepth);

    // data chunk header
    push_fourCC    (wav, "data");
    push_uint32_le (wav, dataChunkSize);

    // insert silence into data chunk
    wav.resize (wav.size() + dataChunkSize, 0);

    // padding byte if odd
    if (dataChunkSize % 2) wav.push_back (0);

    return wav;
}

#endif
