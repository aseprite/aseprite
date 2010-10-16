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
 *      Dynamic driver list helpers.
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"



/* count_drivers:
 *  Returns the number of drivers in a list, not including the sentinel.
 */
static int count_drivers(_DRIVER_INFO *drvlist)
{
   _DRIVER_INFO *start = drvlist;
   while (drvlist->driver) drvlist++;
   return drvlist - start;
}



/* _create_driver_list:
 *  Creates a new driver list and returns it. Returns NULL on error.
 */
_DRIVER_INFO *_create_driver_list(void)
{
   _DRIVER_INFO *drv;
	
   drv = _AL_MALLOC(sizeof(struct _DRIVER_INFO));

   if (drv) {
      drv->id = 0;
      drv->driver = NULL;
      drv->autodetect = FALSE;
   }

   return drv;
}



/* _destroy_driver_list:
 *  Frees a driver list.
 */
void _destroy_driver_list(_DRIVER_INFO *drvlist)
{
   _AL_FREE(drvlist);
}



/* _driver_list_append_driver:
 *  Adds a driver to the end of a driver list.
 */
void _driver_list_append_driver(_DRIVER_INFO **drvlist, int id, void *driver, int autodetect)
{
   _DRIVER_INFO *drv;
   int c;
    
   ASSERT(*drvlist);

   c = count_drivers(*drvlist);

   drv = _AL_REALLOC(*drvlist, sizeof(_DRIVER_INFO) * (c+2));
   if (!drv)
      return;

   drv[c].id = id;
   drv[c].driver = driver;
   drv[c].autodetect = autodetect;
   drv[c+1].id = 0;
   drv[c+1].driver = NULL;
   drv[c+1].autodetect = FALSE;

   *drvlist = drv;
}



/* _driver_list_prepend_driver:
 *  Adds a driver to the start of a driver list.
 */
void _driver_list_prepend_driver(_DRIVER_INFO **drvlist, int id, void *driver, int autodetect)
{
   _DRIVER_INFO *drv;
   int c;
    
   ASSERT(*drvlist);

   c = count_drivers(*drvlist);

   drv = _AL_REALLOC(*drvlist, sizeof(_DRIVER_INFO) * (c+2));
   if (!drv)
      return;
   
   memmove(drv+1, drv, sizeof(_DRIVER_INFO) * (c+1));

   drv[0].id = id;
   drv[0].driver = driver;
   drv[0].autodetect = autodetect;

   *drvlist = drv;
}



/* _driver_list_append_list:
 *  Add drivers from one list to another list.
 */
void _driver_list_append_list(_DRIVER_INFO **drvlist, _DRIVER_INFO *srclist)
{
   ASSERT(*drvlist);
   ASSERT(srclist);

   while (srclist->driver) {
      _driver_list_append_driver(drvlist, srclist->id, srclist->driver, srclist->autodetect);
      srclist++;
   }
}
