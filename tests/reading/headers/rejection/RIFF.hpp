// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_RIFF_hpp
#define wavplusplus_tests_RIFF_hpp

#include <cstdint>
#include <vector>

inline void RiffIdNotFound (TestLog & log)
{
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8);
    bytes[0] = 'J'; bytes[1] = 'I'; bytes[2] = 'F'; bytes[3] = 'F'; // JIFF
    TestForReadError (bytes, log, "Header Rejection - RIFF chunk, first four bytes not RIFF", wav::ReadErrorCode::RiffIdNotFound);
}
inline void RiffSubformatNotWave (TestLog & log)
{
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8);
    bytes[8] = 'S'; bytes[9] = 'A'; bytes[10] = 'V'; bytes[11] = 'E'; // SAVE
    TestForReadError (bytes, log, "Header Rejection - RIFF chunk, subformat not WAVE", wav::ReadErrorCode::RiffSubformatNotWave);
}
inline void MultipleRiffChunks (TestLog & log)
{
    // Second "RIFF" must sit inside the first RIFF envelope or trigger a different
    // ReadError.
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8);
    const auto duplicate {bytes};
    bytes.insert (bytes.end(), duplicate.begin(), duplicate.end());
    UpdateRIFFChunkSize (bytes);
    TestForReadError (bytes, log, "Header Rejection - RIFF chunk, multiple RIFF chunks", wav::ReadErrorCode::MultipleRiffChunks);
}
inline void RiffEndExceedsFileSize (TestLog & log)
{
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 24, 24, 1, 96000, 8);
    const auto correctRiffSize {static_cast<uint32_t> (bytes.size()) - 8u};
    OverwriteUint32LE (bytes, 4, correctRiffSize + 2u); // new RIFF end is 2 bytes past EOF
    TestForReadError (bytes, log, "Header Rejection - RIFF chunk, declared size extends past actual EOF",
                      wav::ReadErrorCode::RiffEndExceedsFileSize);
}
inline void IncompleteRiffHeader (TestLog & log)
{
    std::vector<uint8_t> bytes;
    PushFourCC   (bytes, "RIFF");
    PushUint32LE (bytes, 4u);
    PushFourCC   (bytes, "WAVE");
    bytes.pop_back (); // now only 11 bytes
    TestForReadError (bytes, log, "Header Rejection - RIFF header fewer than 12 bytes", wav::ReadErrorCode::IncompleteRiffHeader);
}

inline void RunRiffChunkTests (TestLog & log)
{
    RiffIdNotFound (log);
    RiffSubformatNotWave (log);
    MultipleRiffChunks (log);
    RiffEndExceedsFileSize (log);
    IncompleteRiffHeader (log);
}

#endif
