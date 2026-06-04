// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_fact_hpp
#define wavplusplus_tests_fact_hpp

inline void TestFact (TestLog & log)
{
    const auto filename {"test_fact_header_parsing.wav"};
    constexpr auto formatCode {wav::FormatCode::FloatPCM};
    constexpr uint16_t channels {2};
    constexpr uint16_t bitDepth {32};
    constexpr uint32_t frames   {8};

    // Construct RIFF, fmt, and data chunks:
    auto bytes = buildPCMWAVEFORMAT (formatCode, bitDepth, bitDepth, channels, 48000, frames);

    // Add fact chunk:
    push_fact_chunk (bytes, frames);
    // Update RIFF chunk size to reflect fact chunk size:
    overwriteUint32LE (bytes, 4, static_cast<uint32_t> (bytes.size() - 8));

    writeFile (filename, bytes);

    std::string failures;
    try
    {
        wav::ChunkInfo info {filename};
        Compare (info.fact.present  == true,   "fact chunk not detected",  failures);
        Compare (info.fact.size     == 4,      "unexpected fact.size",     failures);
        Compare (info.fact.fileSize == frames, "unexpected fact.fileSize", failures);
    }
    catch (wav::ReadError const & e)
    {
        failures = std::string {"wav::ReadError - "} + e.what();
    }
    const auto suffix {failures.empty() ? "" : ", failures: " + failures};
    log.Record (failures.empty(), std::format ("Read Header - optional chunks - fact parsed correctly{}", suffix));

    std::remove (filename);
}

#endif
