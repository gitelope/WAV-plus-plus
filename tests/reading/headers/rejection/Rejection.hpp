// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_Rejection_hpp
#define wavplusplus_tests_Rejection_hpp

#include <limits>

void TestOverlargeDataChunkSize (TestLog &);

void TestRejection (TestLog & log)
{
    TestOverlargeDataChunkSize (log);
}

inline void TestOverlargeDataChunkSize (TestLog & log)
{
    const auto filename {"test_corrupt_data_size.wav"};

    // Actual data chunk size:
    // 1 byte per sample x 1 channel x 8 frames = 8 bytes
    constexpr uint16_t bitDepth {8};
    constexpr uint16_t channels {1};
    constexpr uint32_t frames {8};

    auto bytes = buildWAVEFORMATEX (wav::FormatCode::LinearPCM, bitDepth, bitDepth, channels, 44100, frames);

    constexpr uint32_t overlargeDataChunkSize {std::numeric_limits<uint32_t>::max()};

    // Locate the "data" chunk ID and overwrite the four-byte field that follows:
    for (size_t i = 0; i + 8 <= bytes.size(); ++i)
    {
        if (bytes[i] == 'd' && bytes[i+1] == 'a' && bytes[i+2] == 't' && bytes[i+3] == 'a')
        {
            bytes[i+4] = static_cast<uint8_t> (overlargeDataChunkSize      );
            bytes[i+5] = static_cast<uint8_t> (overlargeDataChunkSize >>  8);
            bytes[i+6] = static_cast<uint8_t> (overlargeDataChunkSize >> 16);
            bytes[i+7] = static_cast<uint8_t> (overlargeDataChunkSize >> 24);
            break;
        }
    }

    writeFile (filename, bytes);

    bool threwReadError {false};

    try {wav::Reader reader {filename};}
    catch (wav::ReadError const &) {threwReadError = true;}

    log.Record (threwReadError, "Header Rejection - data chunk purports size extending past actual EOF");
    std::remove (filename);
}
#endif
