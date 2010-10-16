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
 *      List of QNX drivers.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintqnx.h"

#ifndef ALLEGRO_QNX
#error Something is wrong with the makefile
#endif


_DRIVER_INFO _keyboard_driver_list[] =
{
   { KEYBOARD_QNX,      &keyboard_qnx,      TRUE  },
   { 0,                 NULL,               0     }
};


_DRIVER_INFO _timer_driver_list[] =
{
   { TIMERDRV_UNIX_PTHREADS, &timerdrv_unix_pthreads, TRUE  },
   { 0,                      NULL,                    0     }
};


_DRIVER_INFO _mouse_driver_list[] =
{
   { MOUSE_QNX,         &mouse_qnx,         TRUE  },
   { 0,                 NULL,               0     }
};


BEGIN_GFX_DRIVER_LIST
   { GFX_PHOTON_ACCEL,      &gfx_photon_accel,       TRUE  },
   { GFX_PHOTON_SOFT,       &gfx_photon_soft,        TRUE  },
   { GFX_PHOTON_SAFE,       &gfx_photon_safe,        TRUE  },
   { GFX_PHOTON_WIN,        &gfx_photon_win,         TRUE  },
END_GFX_DRIVER_LIST


BEGIN_DIGI_DRIVER_LIST
   { DIGI_ALSA,         &digi_alsa,         TRUE  },
END_DIGI_DRIVER_LIST


BEGIN_MIDI_DRIVER_LIST
   { MIDI_ALSA,         &midi_alsa,         TRUE  },
   { MIDI_DIGMID,       &midi_digmid,       TRUE  },
END_MIDI_DRIVER_LIST


BEGIN_JOYSTICK_DRIVER_LIST
END_JOYSTICK_DRIVER_LIST
