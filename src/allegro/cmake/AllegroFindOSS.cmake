# - Find Open Sound System
#
#  OSS_FOUND       - True if OSS headers found.

# This file is Allegro-specific and requires the following variables to be
# set elsewhere:
#   ALLEGRO_HAVE_MACHINE_SOUNDCARD_H
#   ALLEGRO_HAVE_LINUX_SOUNDCARD_H
#   ALLEGRO_HAVE_SYS_SOUNDCARD_H
#   ALLEGRO_HAVE_SOUNDCARD_H


if(OSS_INCLUDE_DIR)
    # Already in cache, be silent
    set(OSS_FIND_QUIETLY TRUE)
endif(OSS_INCLUDE_DIR)

set(CMAKE_REQUIRED_DEFINITIONS)

if(ALLEGRO_HAVE_SOUNDCARD_H OR ALLEGRO_HAVE_SYS_SOUNDCARD_H OR
        ALLEGRO_HAVE_MACHINE_SOUNDCARD_H OR ALLEGRO_LINUX_SYS_SOUNDCARD_H)

    if(ALLEGRO_HAVE_MACHINE_SOUNDCARD_H)
        set(CMAKE_REQUIRED_DEFINITIONS -DALLEGRO_HAVE_MACHINE_SOUNDCARD_H)
    endif(ALLEGRO_HAVE_MACHINE_SOUNDCARD_H)

    if(ALLEGRO_HAVE_LINUX_SOUNDCARD_H)
        set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS}
            -DALLEGRO_HAVE_LINUX_SOUNDCARD_H)
    endif(ALLEGRO_HAVE_LINUX_SOUNDCARD_H)

    if(ALLEGRO_HAVE_SYS_SOUNDCARD_H)
        set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS}
            -DALLEGRO_HAVE_SYS_SOUNDCARD_H)
    endif(ALLEGRO_HAVE_SYS_SOUNDCARD_H)

    if(ALLEGRO_HAVE_SOUNDCARD_H)
        set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS}
            -DALLEGRO_HAVE_SOUNDCARD_H)
    endif(ALLEGRO_HAVE_SOUNDCARD_H)

    check_c_source_compiles("
        #ifdef ALLEGRO_HAVE_SOUNDCARD_H
        #include <soundcard.h>
        #endif
        #ifdef ALLEGRO_HAVE_SYS_SOUNDCARD_H
        #include <sys/soundcard.h>
        #endif
        #ifdef ALLEGRO_HAVE_LINUX_SOUNDCARD_H
        #include <linux/soundcard.h>
        #endif
        #ifdef ALLEGRO_HAVE_MACHINE_SOUNDCARD_H
        #include <machine/soundcard.h>
        #endif
        int main(void) {
            audio_buf_info abi;
            return 0;
        }"
        OSS_COMPILES
    )

    set(CMAKE_REQUIRED_DEFINITIONS)

endif(ALLEGRO_HAVE_SOUNDCARD_H OR ALLEGRO_HAVE_SYS_SOUNDCARD_H OR
    ALLEGRO_HAVE_MACHINE_SOUNDCARD_H OR ALLEGRO_LINUX_SYS_SOUNDCARD_H)

# Handle the QUIETLY and REQUIRED arguments and set OSS_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OSS DEFAULT_MSG
    OSS_COMPILES)

mark_as_advanced(OSS_COMPILES)
