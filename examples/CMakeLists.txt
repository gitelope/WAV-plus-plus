
foreach (name IN ITEMS read write inspect) # <-- add new examples here
    add_executable (example_${name} ${name}.cpp)
    target_link_libraries (example_${name} PRIVATE wav_plus_plus)
    target_compile_options (example_${name} PRIVATE ${wav_plus_plus_warning_flags})
    set_target_properties (example_${name} PROPERTIES CXX_EXTENSIONS OFF)
endforeach ()
