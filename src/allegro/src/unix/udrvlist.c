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
 *      Dynamic driver lists shared by Unixy system drivers.
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"



_DRIVER_INFO *_unix_gfx_driver_list = 0;



/* _unix_driver_lists_init:
 *  Initialise driver lists.
 */
void _unix_driver_lists_init(void)
{
   _unix_gfx_driver_list = _create_driver_list();
   if (_unix_gfx_driver_list)
      _driver_list_append_list(&_unix_gfx_driver_list, _gfx_driver_list);
}



/* _unix_driver_lists_shutdown:
 *  Free driver lists.
 */
void _unix_driver_lists_shutdown(void)
{
   if (_unix_gfx_driver_list) {
      _destroy_driver_list(_unix_gfx_driver_list);
      _unix_gfx_driver_list = 0;
   }
}



/* _unix_register_gfx_driver:
 *  Used by modules to register graphics drivers.
 */
void _unix_register_gfx_driver(int id, GFX_DRIVER *driver, int autodetect, int priority)
{
   if (priority)
      _driver_list_prepend_driver(&_unix_gfx_driver_list, id, driver, autodetect);
   else
      _driver_list_append_driver(&_unix_gfx_driver_list, id, driver, autodetect);
}
