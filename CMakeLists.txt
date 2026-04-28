cmake_minimum_required (VERSION 3.21)

project (wav_plus_plus
    VERSION 0.1.0
    LANGUAGES CXX
)

# Build tests and examples by default only when wav_plus_plus is the top-level project.
# When included via add_subdirectory() in another project, these default to OFF to avoid
# polluting the parent build with WAV++'s test and examples.
option (WAV_PLUS_PLUS_BUILD_TESTS    "Build wav_plus_plus tests"    ${PROJECT_IS_TOP_LEVEL})
option (WAV_PLUS_PLUS_BUILD_EXAMPLES "Build wav_plus_plus examples" ${PROJECT_IS_TOP_LEVEL})

if (WAV_PLUS_PLUS_BUILD_TESTS)
    enable_testing ()
endif ()

add_library (wav_plus_plus INTERFACE)
target_include_directories (wav_plus_plus INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# C++20
target_compile_features (wav_plus_plus INTERFACE cxx_std_20)

if (PROJECT_IS_TOP_LEVEL)
    set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif ()

# ---- Warning flags for test and examples ----
if (MSVC)
    set (wav_plus_plus_warning_flags /W4 /permissive-)
else ()
    set (wav_plus_plus_warning_flags
        -Wall
        -Wextra
        -Wpedantic
        -Wnarrowing
        -Wconversion
        -Wcast-align
        -Wcast-qual
        -Wold-style-cast
        -Wshadow
        -Wunused-but-set-variable
        -Wunused-but-set-parameter
        -Wredundant-decls
        -Wzero-as-null-pointer-constant
        -Wuninitialized
        -Wunreachable-code
        -Wundef
        -Wmissing-field-initializers
        -Wtype-limits
        -Wdeprecated
        -Wsign-promo
        -Wnon-virtual-dtor
        -Wdouble-promotion
        -Wformat-nonliteral
        -Wmissing-braces
        -Wreorder
        -Wsign-compare
    )
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        list (APPEND wav_plus_plus_warning_flags
            -Wdocumentation
            -Wimplicit-int-conversion
            -Wimplicit-int-float-conversion
        )
    endif ()
endif ()

# ---- Header listing for browsing in an IDE ----
if (PROJECT_IS_TOP_LEVEL)
    #          Public
    set (wav_plus_plus_public_headers
        include/wav-plus-plus/ChunkInfo.hpp
        include/wav-plus-plus/Exceptions.hpp
        include/wav-plus-plus/FileInfo.hpp
        include/wav-plus-plus/Formats.hpp
        include/wav-plus-plus/Reader.hpp
        include/wav-plus-plus/Writer.hpp
    )
    #          Private
    set (wav_plus_plus_private_headers
        include/wav-plus-plus/private/AudioSummary.hpp
        include/wav-plus-plus/private/BufferSizing.hpp
        include/wav-plus-plus/private/Byteswap.hpp
        include/wav-plus-plus/private/ChunkInfoImplementation.hpp
        include/wav-plus-plus/private/FileInfoImplementation.hpp
        include/wav-plus-plus/private/FormatsImplementation.hpp
        include/wav-plus-plus/private/QuantizationConstants.hpp
        include/wav-plus-plus/private/ReaderImplementation.hpp
        include/wav-plus-plus/private/ReadingBytes.hpp
        include/wav-plus-plus/private/WriterImplementation.hpp
        include/wav-plus-plus/private/int24.hpp
    )
    #          All
    set (wav_plus_plus_all_headers
        ${wav_plus_plus_public_headers}
        ${wav_plus_plus_private_headers}
    )
    source_group (TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${wav_plus_plus_all_headers})
    # IDE-only target so include/ appears once in navigator
    add_custom_target (wav_plus_plus_headers SOURCES ${wav_plus_plus_all_headers})
endif ()

# ---- Examples ----
if (WAV_PLUS_PLUS_BUILD_EXAMPLES)
    add_subdirectory (examples)
endif ()

# ---- Tests ----
if (WAV_PLUS_PLUS_BUILD_TESTS)
    add_subdirectory (tests)
endif ()

