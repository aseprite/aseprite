# - Find DirectInput
# Find the DirectInput includes and libraries
#
#  DDRAW_INCLUDE_DIR - where to find ddraw.h
#  DDRAW_LIBRARIES   - List of libraries when using ddraw.
#  DDRAW_FOUND       - True if ddraw found.

if(DDRAW_INCLUDE_DIR)
    # Already in cache, be silent
    set(DDRAW_FIND_QUIETLY TRUE)
endif(DDRAW_INCLUDE_DIR)

find_path(DDRAW_INCLUDE_DIR ddraw.h
    PATH $ENV{DXSDK_DIR}/Include
    )

find_library(DDRAW_LIBRARY
    NAMES ddraw
    PATHS "$ENV{DXSDK_DIR}/Lib/$ENV{PROCESSOR_ARCHITECTURE}"
    )

# Handle the QUIETLY and REQUIRED arguments and set DDRAW_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DDRAW DEFAULT_MSG
    DDRAW_INCLUDE_DIR DDRAW_LIBRARY)

if(DDRAW_FOUND)
    set(DDRAW_LIBRARIES ${DDRAW_LIBRARY})
else(DDRAW_FOUND)
    set(DDRAW_LIBRARIES)
endif(DDRAW_FOUND)

mark_as_advanced(DDRAW_INCLUDE_DIR DDRAW_LIBRARY)
