// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_rejection_fact_hpp
#define wavplusplus_tests_rejection_fact_hpp

#include <cstdint>

inline void FactChunkSizeNotFour (TestLog & log)
{
    constexpr uint32_t frames {8};
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::FloatPCM, 32, 32, 2, 48000, frames);
    const auto factChunkStart {bytes.size()};
    PushFactChunk (bytes, frames);
    // Rewrite fact chunk size to invalid value of 2.
    // (Must be smaller than 4, not bigger, to not trigger DeclaredChunkEndExceedsRiffEnd error.)
    OverwriteUint32LE (bytes, factChunkStart + 4, 2u);
    UpdateRIFFChunkSize (bytes);
    TestForReadError (bytes, log, "Header Rejection - fact chunk, size not 4", wav::ReadErrorCode::FactChunkSizeNotFour);
}
inline void MultipleFactChunks (TestLog & log)
{
    constexpr uint32_t frames {8};
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, frames);
    PushFactChunk (bytes, frames);
    PushFactChunk (bytes, frames);
    UpdateRIFFChunkSize (bytes);
    TestForReadError (bytes, log, "Header Rejection - fact chunk, multiple fact chunks", wav::ReadErrorCode::MultipleFactChunks);
}

inline void RunFactChunkTests (TestLog & log)
{
    FactChunkSizeNotFour (log);
    MultipleFactChunks (log);
}

#endif
