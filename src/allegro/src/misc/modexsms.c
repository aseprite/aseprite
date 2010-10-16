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
 *      Hack to handle split_modex_screen().
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"

#include "modexsms.h"


#ifdef GFX_MODEX

/* split_modex_screen() should be available whenever GFX_MODEX is defined,
 * but the ModeX driver itself either may not be available on the given
 * platform or may reside in a module.  The right way to handle this would be
 * to add another method to the GFX_VTABLE, but in this case the method would
 * be useless for all other drivers.  Also split_modex_screen() is going to
 * be removed soon anyway.  Hence this hack: we set _split_modex_screen_ptr
 * to point to the real function when the ModeX driver is initialised and
 * unset it when it is shut down.
 */

void (*_split_modex_screen_ptr)(int);

void split_modex_screen(int line)
{
   if (_split_modex_screen_ptr)
      _split_modex_screen_ptr(line);
}

#endif
