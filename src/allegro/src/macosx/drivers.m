/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      List of MacOS X drivers.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintosx.h"

#ifndef ALLEGRO_MACOSX
#error Something is wrong with the makefile
#endif


_DRIVER_INFO _keyboard_driver_list[] =
{
   { KEYBOARD_MACOSX,         &keyboard_macosx,         TRUE  },
   { 0,                       NULL,                     0     }
};


_DRIVER_INFO _timer_driver_list[] =
{
   { TIMERDRV_UNIX_PTHREADS,  &timerdrv_unix_pthreads,  TRUE  },
   { 0,                       NULL,                     0     }
};


_DRIVER_INFO _mouse_driver_list[] =
{
   { MOUSE_MACOSX,            &mouse_macosx,            TRUE  },
   { 0,                       NULL,                     0     }
};


BEGIN_GFX_DRIVER_LIST
{   GFX_QUARTZ_FULLSCREEN,    &gfx_quartz_full,         TRUE  },
{   GFX_QUARTZ_WINDOW,        &gfx_quartz_window,       TRUE  },
END_GFX_DRIVER_LIST


BEGIN_DIGI_DRIVER_LIST
{   DIGI_CORE_AUDIO,          &digi_core_audio,         TRUE  },
{   DIGI_SOUND_MANAGER,       &digi_sound_manager,      TRUE  },
END_DIGI_DRIVER_LIST


BEGIN_MIDI_DRIVER_LIST
{   MIDI_CORE_AUDIO,          &midi_core_audio,         TRUE  },
{   MIDI_QUICKTIME,           &midi_quicktime,          TRUE  },
END_MIDI_DRIVER_LIST


BEGIN_JOYSTICK_DRIVER_LIST
{   JOYSTICK_HID,             &joystick_hid,            TRUE  },
END_JOYSTICK_DRIVER_LIST
