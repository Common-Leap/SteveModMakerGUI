if(NOT DEFINED TARGET_RUNTIME_DLLS OR TARGET_RUNTIME_DLLS STREQUAL "")
    return()
endif()

string(REPLACE "|" ";" runtime_dlls "${TARGET_RUNTIME_DLLS}")
foreach(runtime_dll IN LISTS runtime_dlls)
    if(EXISTS "${runtime_dll}")
        get_filename_component(runtime_name "${runtime_dll}" NAME)
        file(COPY_FILE
            "${runtime_dll}"
            "${TARGET_FILE_DIR}/${runtime_name}"
            ONLY_IF_DIFFERENT
        )
    endif()
endforeach()
