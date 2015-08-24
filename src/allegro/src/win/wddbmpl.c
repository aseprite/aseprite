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
 *      DirectDraw surface list management.
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */


#include "wddraw.h"

#define PREFIX_I                "al-wddbmpl INFO: "
#define PREFIX_W                "al-wddbmpl WARNING: "
#define PREFIX_E                "al-wddbmpl ERROR: "


/* double linked list */
static DDRAW_SURFACE *ddraw_surface_list = NULL;



/* register_ddraw_surface:
 *  Adds a surface to the linked list.
 */
void register_ddraw_surface(DDRAW_SURFACE *surf)
{
   _enter_gfx_critical();

   surf->next = ddraw_surface_list;
   surf->prev = NULL;

   if (ddraw_surface_list)
      ddraw_surface_list->prev = surf;

   ddraw_surface_list = surf;

   _exit_gfx_critical();
}



/* unregister_ddraw_surface:
 *  Removes a surface from the linked list.
 */
void unregister_ddraw_surface(DDRAW_SURFACE *surf)
{
   DDRAW_SURFACE *item;

   _enter_gfx_critical();

   item = ddraw_surface_list;

   while (item) {
      if (item == surf) {
         /* surface found, unlink now */
         if (item->next)
            item->next->prev = item->prev;
         if (item->prev)
            item->prev->next = item->next;
         if (ddraw_surface_list == item)
            ddraw_surface_list = item->next;

         item->next = NULL;
         item->prev = NULL;
         break;
      }

      item = item->next;
   }

   _exit_gfx_critical();
}



/* unregister_all_ddraw_surfaces:
 *  Removes all surfaces from the linked list.
 */
void unregister_all_ddraw_surfaces(void)
{
   DDRAW_SURFACE *item, *next_item;

   _enter_gfx_critical();

   next_item = ddraw_surface_list;

   while (next_item) {
      item = next_item;
      next_item = next_item->next;
      item->next = NULL;
      item->prev = NULL;
   }

   ddraw_surface_list = NULL;

   _exit_gfx_critical();
}



/* restore_all_ddraw_surfaces:
 *  Restores all the surfaces. Returns 0 on success or -1 on failure,
 *  in which case restoring is stopped at the first failure.
 */
int restore_all_ddraw_surfaces(void)
{
   DDRAW_SURFACE *item;

   _enter_gfx_critical();

   item = ddraw_surface_list;
   while (item) {
      if (gfx_directx_restore_surface(item) != 0) {
         _exit_gfx_critical();
         return -1;
      }
      item = item->next;
   }

   _exit_gfx_critical();

   _TRACE(PREFIX_I "all DirectDraw surfaces restored\n");

   return 0;
}
