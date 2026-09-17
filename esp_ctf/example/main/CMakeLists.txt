idf_component_register(SRCS "app_main.c"
                       PRIV_REQUIRES esp_ctf esp_trace app_trace
                       INCLUDE_DIRS "")

if(CONFIG_ESP_TRACE_FUNCTION_TRACE)
    target_compile_options(${COMPONENT_LIB} PRIVATE -finstrument-functions)
endif()
