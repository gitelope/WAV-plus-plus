// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_writing_FileOpenFailure_hpp
#define wavplusplus_tests_writing_FileOpenFailure_hpp

#include <cassert>
#include <filesystem>

namespace writing
{

inline void FailedToOpenFileForWriting (TestLog & log)
{
    const std::filesystem::path filepath {"does_not_exist_dir/out.wav"};

    assert (!std::filesystem::exists (filepath));
    assert (!std::filesystem::exists (filepath.parent_path()));

    bool passes {false};
    try
    {
        wav::Writer writer {filepath, wav::ExportFormat::LinearPCM_16, 44100, 1};
    }
    catch (wav::WriteError const & e)
    {
        passes = (e.code() == wav::WriteErrorCode::FailedToOpenFileForWriting);
    }

    log.Record (passes, "Writer FileOpenFailure - failed to open file for writing");
}

inline void TestFileOpenFailure (TestLog & log)
{
    FailedToOpenFileForWriting (log);
}

} // namespace writing

#endif
