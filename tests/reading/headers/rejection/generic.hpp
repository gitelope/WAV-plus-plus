// WAV++ (c) 2025 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_generic_hpp
#define wavplusplus_tests_generic_hpp

inline void DeclaredChunkEndExceedsRiffEnd (TestLog & log)
{
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8);
    PushFourCC (bytes, "chnk");
    PushUint32LE (bytes, 1u); // no body bytes; declared end (incl. pad) past riffEnd
    UpdateRIFFChunkSize (bytes);
    TestForReadError (bytes, log,
                      "Header Rejection - declared sub-chunk end exceeds RIFF end",
                      wav::ReadErrorCode::DeclaredChunkEndExceedsRiffEnd);
}
inline void IncompleteChunkHeader (TestLog & log)
{
    auto bytes = buildPCMWAVEFORMAT (wav::FormatCode::LinearPCM, 8, 8, 1, 44100, 8);
    // Add four bytes which is not enough for a chunk ID and size field
    bytes.push_back (0);
    bytes.push_back (0);
    bytes.push_back (0);
    bytes.push_back (0);
    UpdateRIFFChunkSize (bytes);
    TestForReadError (bytes, log,
                      "Header Rejection - RIFF chunk, incomplete sub-chunk header",
                      wav::ReadErrorCode::IncompleteChunkHeader);
}

inline void RunGenericChunkTests (TestLog & log)
{
    DeclaredChunkEndExceedsRiffEnd (log);
    IncompleteChunkHeader (log);
}

#endif
