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
 *      Linux console video driver list.
 *
 *      By George Foot.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"


/* list the available drivers */
_DRIVER_INFO _linux_gfx_driver_list[] =
{
#if (defined ALLEGRO_LINUX_FBCON) && (!defined ALLEGRO_WITH_MODULES)
   {  GFX_FBCON,    &gfx_fbcon,    TRUE  },
#endif
#ifdef ALLEGRO_LINUX_VBEAF
   {  GFX_VBEAF,    &gfx_vbeaf,    TRUE  },
#endif
#if (defined ALLEGRO_LINUX_VGA) && (!defined ALLEGRO_WITH_MODULES)
   {  GFX_VGA,      &gfx_vga,      TRUE  },
/* {  GFX_MODEX,    &gfx_modex,    TRUE  }, */
#endif
#if (defined ALLEGRO_LINUX_SVGALIB) && (!defined ALLEGRO_WITH_MODULES)
   {  GFX_SVGALIB,  &gfx_svgalib,  FALSE },
#endif
   {  0,            NULL,          0     }
};

