if(CONFIG_ESP_TRACE_LIB_EXTERNAL)
    set(srcs "src/adapter_encoder_ctf.c")
    set(include_dirs "include")

    configure_file(linker.lf.in ${CMAKE_CURRENT_BINARY_DIR}/linker.lf)

    idf_component_register(SRCS "${srcs}"
                        INCLUDE_DIRS "${include_dirs}"
                        PRIV_INCLUDE_DIRS "private_include"
                        REQUIRES     esp_trace
                        LDFRAGMENTS ${CMAKE_CURRENT_BINARY_DIR}/linker.lf
                        WHOLE_ARCHIVE TRUE # Link all symbols so self-registering adapters (ESP_TRACE_REGISTER_*) are not discarded
                        )

    # Let freertos find esp_trace_freertos_impl.h
    idf_component_get_property(freertos_lib freertos COMPONENT_LIB)
    target_include_directories(${freertos_lib} INTERFACE ${include_dirs})

    if(CONFIG_ESP_CTF_OPTIMIZE_SPEED)
        target_compile_options(${COMPONENT_LIB} PRIVATE -O2)
    endif()
else()
    idf_component_register(REQUIRES esp_trace)
endif()
