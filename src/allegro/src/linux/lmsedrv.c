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
 *      Linux console mouse driver list.
 *
 *      By George Foot.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"

/* list the available drivers */
_DRIVER_INFO _linux_mouse_driver_list[] =
{
   {  MOUSEDRV_LINUX_IPS2,     &mousedrv_linux_ips2,     TRUE  },
   {  MOUSEDRV_LINUX_PS2,      &mousedrv_linux_ps2,      TRUE  },
   {  MOUSEDRV_LINUX_MS,       &mousedrv_linux_ms,       TRUE  },
   {  MOUSEDRV_LINUX_IMS,      &mousedrv_linux_ims,      TRUE  },
   {  MOUSEDRV_LINUX_GPMDATA,  &mousedrv_linux_gpmdata,  TRUE  },
#ifdef ALLEGRO_HAVE_LINUX_INPUT_H
   {  MOUSEDRV_LINUX_EVDEV,    &mousedrv_linux_evdev,    TRUE  },
#endif
   {  MOUSEDRV_NONE,           &mousedrv_none,           TRUE  },
   {  0,                       NULL,                     0     }
};

