// WAV++
// Copyright (c) 2025 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_writing_IsOpen_hpp
#define wavplusplus_tests_writing_IsOpen_hpp

#include <cstdio>

inline void TestIsOpen (TestLog & log)
{
    const auto filename {"test_writer_is_open.wav"};

    wav::Writer writer {filename, wav::ExportFormat::LinearPCM_16, 44100, 1};
    const bool openAfterConstruction {writer.isOpen()};
    writer.Close();
    const bool closedAfterClose {!writer.isOpen()};

    log.Record (openAfterConstruction && closedAfterClose,
                "Writer FileOpen - isOpen() true after construction, false after Close()");

    std::remove (filename);
}

#endif
