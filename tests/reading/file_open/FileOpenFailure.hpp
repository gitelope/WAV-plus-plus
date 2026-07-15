// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_FileOpenFailure_hpp
#define wavplusplus_tests_FileOpenFailure_hpp

#include <cassert>
#include <filesystem>

inline void FailedToOpenFileForReading (TestLog & log)
{
    const std::filesystem::path filepath {"does_not_exist.wav"};
    assert (!std::filesystem::exists (filepath));

    bool passes {false};
    try
    {
        wav::Reader reader {filepath};
    }
    catch (wav::ReadError const & e)
    {
        passes = (e.code() == wav::ReadErrorCode::FailedToOpenFileForReading);
    }

    log.Record (passes, "Reader FileOpenFailure - failed to open file for reading");
}

inline void TestFileOpenFailure (TestLog & log)
{
    FailedToOpenFileForReading (log);
}

#endif
