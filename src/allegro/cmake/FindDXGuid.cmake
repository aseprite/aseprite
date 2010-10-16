# - Find dxguid
# Find the dxguid libraries
#
#  DXGUID_LIBRARIES   - List of libraries
#  DXGUID_FOUND       - True if dxguid found.

if(MSVC)
    find_library(DXGUID_LIBRARY NAMES dxguid
        PATHS "C:/Program Files/Microsoft DirectX SDK (August 2008)/Lib"
        PATH_SUFFIXES x86 x64
        )
else()
    find_library(DXGUID_LIBRARY NAMES dxguid)
endif()

# Handle the QUIETLY and REQUIRED arguments and set DXGUID_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DXGUID DEFAULT_MSG
    DXGUID_LIBRARY)

if(DXGUID_FOUND)
    set(DXGUID_LIBRARIES ${DXGUID_LIBRARY})
else(DXGUID_FOUND)
    set(DXGUID_LIBRARIES)
endif(DXGUID_FOUND)

mark_as_advanced(DXGUID_INCLUDE_DIR DXGUID_LIBRARY)
