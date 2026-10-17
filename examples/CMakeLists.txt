# WAV++
# Copyright (c) 2025 David Adams
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

# Turn off some noisy unused variable warnings for examples.
if (MSVC)
    target_compile_options (read PRIVATE /wd4101 /wd4189)
    target_compile_options (write PRIVATE /wd4101)
    target_compile_options (inspect PRIVATE /wd4101)
else ()
    target_compile_options (read PRIVATE -Wno-unused-variable)
endif ()
