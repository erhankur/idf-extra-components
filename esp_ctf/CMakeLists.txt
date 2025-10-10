set(sources "src/adapter_encoder_ctf.c")
set(include_dirs "include")
set(dependencies esp_trace)

idf_component_register(
    SRCS         ${sources}
    INCLUDE_DIRS ${include_dirs}
    REQUIRES     ${dependencies}
    WHOLE_ARCHIVE TRUE
)

# Trick to include the CTF header into esp_trace
get_target_property(esp_trace_tgt idf::esp_trace ALIASED_TARGET)
target_include_directories(${esp_trace_tgt} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
