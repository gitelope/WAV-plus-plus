
foreach (name IN ITEMS read write inspect) # <-- add new examples here
    add_executable (${name} ${name}.cpp)
    target_link_libraries (${name} PRIVATE wav_plus_plus)
    target_compile_options (${name} PRIVATE ${wav_plus_plus_warning_flags})
    set_target_properties (${name} PROPERTIES
        CXX_EXTENSIONS OFF
        FOLDER examples
    )
endforeach ()
