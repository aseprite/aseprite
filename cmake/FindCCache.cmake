# To find CCache compiler cache

include(FindPackageHandleStandardArgs)

find_program(CCache_EXECUTABLE ccache)

if (CCache_EXECUTABLE)
    execute_process(COMMAND "${CCache_EXECUTABLE}" --version
        OUTPUT_VARIABLE CCache_VERSION_OUTPUT
    )

    if (CCache_VERSION_OUTPUT MATCHES "version ([0-9]+\\.[0-9]+\\.[0-9]+)")
        set(CCache_VERSION "${CMAKE_MATCH_1}")
    endif ()
endif (CCache_EXECUTABLE)

find_package_handle_standard_args(CCache
    FOUND_VAR     CCache_FOUND
    REQUIRED_VARS CCache_EXECUTABLE
    VERSION_VAR   CCache_VERSION
)

mark_as_advanced(CCache_EXECUTABLE)
