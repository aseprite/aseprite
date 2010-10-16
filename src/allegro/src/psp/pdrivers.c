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
 *      List of PSP drivers.
 *
 *      By diedel.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"

#ifndef ALLEGRO_PSP
#error Something is wrong with the makefile
#endif


_DRIVER_INFO _system_driver_list[] =
{
   { SYSTEM_PSP,              &system_psp,              TRUE  },
   { SYSTEM_NONE,             &system_none,             FALSE },
   { 0,                       NULL,                     0     }
};


_DRIVER_INFO _keyboard_driver_list[] =
{
   { KEYSIM_PSP,              &keybd_simulator_psp,     TRUE  },
   { 0,                       NULL,                     0     }
};


_DRIVER_INFO _timer_driver_list[] =
{
   { TIMER_PSP,               &timer_psp,               TRUE  },
   { 0,                       NULL,                     0     }
};


_DRIVER_INFO _mouse_driver_list[] =
{
   { MOUSE_PSP,               &mouse_psp,               TRUE  },
   { 0,                       NULL,                     0     }
};


_DRIVER_INFO _gfx_driver_list[] =
{
   { GFX_PSP,                 &gfx_psp,                 TRUE  },
   { 0,                       NULL,                     0     }
};


_DRIVER_INFO _digi_driver_list[] =
{
   { DIGI_PSP,                 &digi_psp,               TRUE  },
   { 0,                        NULL,                    0     }
};


BEGIN_MIDI_DRIVER_LIST
MIDI_DRIVER_DIGMID
END_MIDI_DRIVER_LIST


BEGIN_JOYSTICK_DRIVER_LIST
{   JOYSTICK_PSP,              &joystick_psp,           TRUE  },
END_JOYSTICK_DRIVER_LIST
