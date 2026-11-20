// WAV++
// Copyright (c) 2025 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_writing_IsOpen_hpp
#define wavplusplus_tests_writing_IsOpen_hpp

#include <filesystem>

inline void TestIsOpen (TestLog & log)
{
    const auto filepath {tempPath ("test_writer_is_open.wav")};

    wav::Writer writer {filepath, wav::ExportFormat::LinearPCM_16, 44100, 1};
    const bool openAfterConstruction {writer.isOpen()};
    writer.Close();
    const bool closedAfterClose {!writer.isOpen()};

    log.Record (openAfterConstruction && closedAfterClose,
                "Writer FileOpen - isOpen() true after construction, false after Close()");

    std::filesystem::remove (filepath);
}

#endif
