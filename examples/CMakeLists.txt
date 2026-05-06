# WAV++ (c) 2025-2026 David Adams
# SPDX-License-Identifier: MIT

foreach (name IN ITEMS read write inspect) # <-- add new examples here
    add_executable (${name} ${name}.cpp)
    target_link_libraries (${name} PRIVATE wav_plus_plus)
    target_compile_options (${name} PRIVATE ${wav_plus_plus_warning_flags})
    set_target_properties (${name} PROPERTIES
        CXX_EXTENSIONS OFF
        FOLDER examples
    )
endforeach ()

# read example shows a couple ways to extract metadata but does not make further use
# of the variables so we disable the unused but set variable warning.
if (NOT MSVC)
    target_compile_options (read PRIVATE -Wno-unused-variable)
endif ()
