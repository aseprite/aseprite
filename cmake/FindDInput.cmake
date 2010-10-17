# - Find DirectInput
# Find the DirectInput includes and libraries
#
#  DINPUT_INCLUDE_DIR - where to find dinput.h
#  DINPUT_LIBRARIES   - List of libraries when using dinput.
#  DINPUT_FOUND       - True if dinput found.

if(DINPUT_INCLUDE_DIR)
    # Already in cache, be silent
    set(DINPUT_FIND_QUIETLY TRUE)
endif(DINPUT_INCLUDE_DIR)

find_path(DINPUT_INCLUDE_DIR dinput.h
    PATH $ENV{DXSDK_DIR}/Include
    )

find_library(DINPUT_LIBRARY
    NAMES dinput8
    PATHS "$ENV{DXSDK_DIR}/Lib/$ENV{PROCESSOR_ARCHITECTURE}"
    )

# Handle the QUIETLY and REQUIRED arguments and set DINPUT_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DINPUT DEFAULT_MSG
    DINPUT_INCLUDE_DIR DINPUT_LIBRARY)

if(DINPUT_FOUND)
    set(DINPUT_LIBRARIES ${DINPUT_LIBRARY})
else(DINPUT_FOUND)
    set(DINPUT_LIBRARIES)
endif(DINPUT_FOUND)

mark_as_advanced(DINPUT_INCLUDE_DIR DINPUT_LIBRARY)
