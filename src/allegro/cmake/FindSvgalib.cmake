# - Find svgalib
#
#  SVGALIB_INCLUDE_DIR - where to find vga.h, etc.
#  SVGALIB_LIBRARIES   - List of libraries when using svgalib.
#  SVGALIB_FOUND       - True if vorbis found.

if(SVGALIB_INCLUDE_DIR)
    # Already in cache, be silent
    set(SVGALIB_FIND_QUIETLY TRUE)
endif(SVGALIB_INCLUDE_DIR)
find_path(SVGALIB_INCLUDE_DIR vga.h)
find_library(SVGALIB_LIBRARY NAMES vga)
# Handle the QUIETLY and REQUIRED arguments and set SVGALIB_FOUND
# to TRUE if all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SVGALIB DEFAULT_MSG
    SVGALIB_INCLUDE_DIR SVGALIB_LIBRARY)

mark_as_advanced(SVGALIB_INCLUDE_DIR)
mark_as_advanced(SVGALIB_LIBRARY)

