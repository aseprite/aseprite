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
 *      Display switching interface.
 *
 *      By George Foot.
 *
 *      State saving routines added by Shawn Hargreaves.
 *
 *      Switch callbacks support added by Lorenzo Petrone,
 *      based on code by Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"


#ifdef ALLEGRO_DOS
   static int switch_mode = SWITCH_NONE;
#else
   static int switch_mode = SWITCH_PAUSE;
#endif


/* remember things about the way our bitmaps are set up */
typedef struct BITMAP_INFORMATION
{
   BITMAP *bmp;                           /* the bitmap */
   BITMAP *other;                         /* replacement during switches */
   struct BITMAP_INFORMATION *sibling;    /* linked list of siblings */
   struct BITMAP_INFORMATION *child;      /* tree of sub-bitmaps */
   void *acquire, *release;               /* need to bodge the vtable, too */
   int blit_on_restore;                   /* whether the bitmap contents need to be copied back */
} BITMAP_INFORMATION;

static BITMAP_INFORMATION *info_list = NULL;


int _dispsw_status = SWITCH_NONE;

#define MAX_SWITCH_CALLBACKS  8
static void (*switch_in_cb[MAX_SWITCH_CALLBACKS])(void) = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
static void (*switch_out_cb[MAX_SWITCH_CALLBACKS])(void) = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };



/* set_display_switch_mode:
 *  Sets the display switch mode. Returns zero and unregisters
 *  all callbacks on success, returns non-zero on failure.
 */
int set_display_switch_mode(int mode)
{
   int ret, i;

   if ((!system_driver))
      return -1;

   /* platforms that don't support switching allow SWITCH_NONE */
   if (!system_driver->set_display_switch_mode) {
      if (mode == SWITCH_NONE)
         return 0;
      else
         return -1;
   }

   ret = system_driver->set_display_switch_mode(mode);

   if (ret == 0) {
      /* unregister all callbacks */
      for (i=0; i<MAX_SWITCH_CALLBACKS; i++)
         switch_in_cb[i] = switch_out_cb[i] = NULL;

      switch_mode = mode;
   }

   return ret;
}



/* get_display_switch_mode:
 *  Returns the current display switch mode.
 */
int get_display_switch_mode(void)
{
   return switch_mode;
}



/* set_display_switch_callback:
 *  Registers a display switch callback. The first parameter indicates
 *  the direction (SWITCH_IN or SWITCH_OUT). Returns zero on success,
 *  non-zero on failure (e.g. feature not supported).
 */
int set_display_switch_callback(int dir, void (*cb)(void))
{
   int i;

   if ((dir != SWITCH_IN) && (dir != SWITCH_OUT))
      return -1;

   if ((!system_driver) || (!system_driver->set_display_switch_mode))
      return -1;

   for (i=0; i<MAX_SWITCH_CALLBACKS; i++) {
      if (dir == SWITCH_IN) {
	 if (!switch_in_cb[i]) {
	    switch_in_cb[i] = cb;
	    return 0;
	 }
      }
      else {
	 if (!switch_out_cb[i]) {
	    switch_out_cb[i] = cb;
	    return 0;
	 }
      }
   }

   return -1;
}



/* remove_display_switch_callback:
 *  Unregisters a display switch callback.
 */
void remove_display_switch_callback(void (*cb)(void))
{
   int i;

   for (i=0; i<MAX_SWITCH_CALLBACKS; i++) {
      if (switch_in_cb[i] == cb)
	 switch_in_cb[i] = NULL;

      if (switch_out_cb[i] == cb)
	 switch_out_cb[i] = NULL;
   }
}



/* _switch_in:
 *  Handles a SWITCH_IN event.
 */
void _switch_in(void)
{
   int i;

   for (i=0; i<MAX_SWITCH_CALLBACKS; i++) {
      if (switch_in_cb[i])
	 switch_in_cb[i]();
   }
}



/* _switch_out:
 *  Handles a SWITCH_OUT event.
 */
void _switch_out(void)
{
   int i;

   for (i=0; i<MAX_SWITCH_CALLBACKS; i++) {
      if (switch_out_cb[i])
	 switch_out_cb[i]();
   }
}



/* find_switch_bitmap:
 *  Recursively searches the tree for a particular bitmap.
 */
static BITMAP_INFORMATION *find_switch_bitmap(BITMAP_INFORMATION **head, BITMAP *bmp, BITMAP_INFORMATION ***head_ret)
{
   BITMAP_INFORMATION *info = *head, *kid;

   while (info) {
      if (info->bmp == bmp) {
	 *head_ret = head;
	 return info;
      }

      if (info->child) {
	 kid = find_switch_bitmap(&info->child, bmp, head_ret);
	 if (kid)
	    return kid;
      }

      head = &info->sibling;
      info = *head;
   }

   return NULL;
}



/* _register_switch_bitmap:
 *  Lists this bitmap as an interesting thing to remember during console
 *  switches.
 */
void _register_switch_bitmap(BITMAP *bmp, BITMAP *parent)
{
   BITMAP_INFORMATION *info, *parent_info, **head;

   if (system_driver->display_switch_lock)
      system_driver->display_switch_lock(TRUE, FALSE);

   if (parent) {
      /* add a sub-bitmap */
      parent_info = find_switch_bitmap(&info_list, parent, &head);
      if (!parent_info)
	 goto getout;

      info = _AL_MALLOC(sizeof(BITMAP_INFORMATION));
      if (!info)
	 goto getout;

      info->bmp = bmp;
      info->other = NULL;
      info->sibling = parent_info->child;
      info->child = NULL;
      info->acquire = NULL;
      info->release = NULL;
      info->blit_on_restore = FALSE;

      parent_info->child = info;
   }
   else {
      /* add a new top-level bitmap: must be in the foreground for this! */
      ASSERT(_dispsw_status == SWITCH_NONE);

      info = _AL_MALLOC(sizeof(BITMAP_INFORMATION));
      if (!info)
	 goto getout;

      info->bmp = bmp;
      info->other = NULL;
      info->sibling = info_list;
      info->child = NULL;
      info->acquire = NULL;
      info->release = NULL;
      info->blit_on_restore = FALSE;

      info_list = info;
   }

   getout:

   if (system_driver->display_switch_lock)
      system_driver->display_switch_lock(FALSE, FALSE);
}



/* _unregister_switch_bitmap:
 *  Removes a bitmap from the list of things that need to be saved.
 */
void _unregister_switch_bitmap(BITMAP *bmp)
{
   BITMAP_INFORMATION *info, **head;

   if (system_driver->display_switch_lock)
      system_driver->display_switch_lock(TRUE, FALSE);

   info = find_switch_bitmap(&info_list, bmp, &head);
   if (!info)
      goto getout;

   /* all the sub-bitmaps should be destroyed before we get to here */
   ASSERT(!info->child);

   /* it's not cool to destroy things that have important state */
   ASSERT(!info->other);

   *head = info->sibling;
   _AL_FREE(info);

   getout:

   if (system_driver->display_switch_lock)
      system_driver->display_switch_lock(FALSE, FALSE);
}



/* reconstruct_kids:
 *  Recursive helper to rebuild any sub-bitmaps to point at their new
 *  parents.
 */
static void reconstruct_kids(BITMAP *parent, BITMAP_INFORMATION *info)
{
   int x, y, i;

   while (info) {
      info->bmp->vtable = parent->vtable;
      info->bmp->write_bank = parent->write_bank;
      info->bmp->read_bank = parent->read_bank;
      info->bmp->seg = parent->seg;
      info->bmp->id = parent->id | BMP_ID_SUB;

      x = info->bmp->x_ofs - parent->x_ofs;
      y = info->bmp->y_ofs - parent->y_ofs;

      if (is_planar_bitmap(info->bmp))
	 x /= 4;

      x *= BYTES_PER_PIXEL(bitmap_color_depth(info->bmp));

      for (i=0; i<info->bmp->h; i++)
	 info->bmp->line[i] = parent->line[y+i] + x;

      reconstruct_kids(info->bmp, info->child);
      info = info->sibling;
   }
}



/* fudge_bitmap:
 *  Makes b2 be similar to b1 (duplicate clip settings, ID, etc).
 */
static void fudge_bitmap(BITMAP *b1, BITMAP *b2, int copy)
{
   int s, x1, y1, x2, y2;

   set_clip_state(b2, FALSE);

   if (copy)
      blit(b1, b2, 0, 0, 0, 0, b1->w, b1->h);

   get_clip_rect(b1, &x1, &y1, &x2, &y2);
   s = get_clip_state(b1);

   set_clip_rect(b2, x1, y1, x2, y2);
   set_clip_state(b2, s);
}



/* swap_bitmap_contents:
 *  Now remember boys and girls, don't try this at home!
 */
static void swap_bitmap_contents(BITMAP *b1, BITMAP *b2)
{
   int size = sizeof(BITMAP) + sizeof(char *) * b1->h;
   unsigned char *s = (unsigned char *)b1;
   unsigned char *d = (unsigned char *)b2;
   unsigned char t;
   int c;

   for (c=0; c<size; c++) {
      t = s[c];
      s[c] = d[c];
      d[c] = t;
   }
}



/* save_bitmap_state:
 *  Remember everything important about a screen bitmap.
 *
 *  Note: this must run even for SWITCH_BACKAMNESIA.  With the fbcon driver,
 *  writes to the screen would still show up while we are switched away.
 *  So we let this function run to redirect the screen to a memory bitmap while
 *  we are switched away.  We also let this run for SWITCH_AMNESIA just for
 *  consistency.
 */
static void save_bitmap_state(BITMAP_INFORMATION *info, int switch_mode)
{
   int copy;

   info->other = create_bitmap_ex(bitmap_color_depth(info->bmp), info->bmp->w, info->bmp->h);
   if (!info->other)
      return;

   copy = (switch_mode != SWITCH_AMNESIA) && (switch_mode != SWITCH_BACKAMNESIA);
   fudge_bitmap(info->bmp, info->other, copy);
   info->blit_on_restore = copy;

   info->acquire = info->other->vtable->acquire;
   info->release = info->other->vtable->release;

   info->other->vtable->acquire = info->bmp->vtable->acquire;
   info->other->vtable->release = info->bmp->vtable->release;

   #define INTERESTING_ID_BITS   (BMP_ID_VIDEO | BMP_ID_SYSTEM | \
				  BMP_ID_SUB | BMP_ID_MASK)

   info->other->id = (info->bmp->id & INTERESTING_ID_BITS) | 
		     (info->other->id & ~INTERESTING_ID_BITS);

   swap_bitmap_contents(info->bmp, info->other);
}



/* _save_switch_state:
 *  Saves the graphics state before a console switch.
 */
void _save_switch_state(int switch_mode)
{
   BITMAP_INFORMATION *info = info_list;
   int hadmouse;

   if (!screen)
      return;

   if (_al_linker_mouse && 
       is_same_bitmap(*(_al_linker_mouse->mouse_screen_ptr), screen)) {
      _al_linker_mouse->show_mouse(NULL);
      hadmouse = TRUE;
   }
   else
      hadmouse = FALSE;

   while (info) {
      save_bitmap_state(info, switch_mode);
      reconstruct_kids(info->bmp, info->child);
      info = info->sibling;
   }

   _dispsw_status = switch_mode;

   if (hadmouse)
      _al_linker_mouse->show_mouse(screen);
}



/* restore_bitmap_state:
 *  The King's Men are quite good at doing this with bitmaps...
 */
static void restore_bitmap_state(BITMAP_INFORMATION *info)
{
   if (info->other) {
      swap_bitmap_contents(info->other, info->bmp);
      info->other->vtable->acquire = info->acquire;
      info->other->vtable->release = info->release;
      info->other->id &= ~INTERESTING_ID_BITS;
      fudge_bitmap(info->other, info->bmp, info->blit_on_restore);
      destroy_bitmap(info->other);
      info->other = NULL;
   }
   else
      clear_bitmap(info->bmp);
}



/* _restore_switch_state:
 *  Restores the graphics state after a console switch.
 */
void _restore_switch_state(void)
{
   BITMAP_INFORMATION *info = info_list;
   int hadmouse, hadtimer;

   if (!screen)
      return;

   if (_al_linker_mouse &&
       is_same_bitmap(*(_al_linker_mouse->mouse_screen_ptr), screen)) {
      _al_linker_mouse->show_mouse(NULL);
      hadmouse = TRUE;
   }
   else
      hadmouse = FALSE;

   hadtimer = _timer_installed;
   _timer_installed = FALSE;

   while (info) {
      restore_bitmap_state(info);
      reconstruct_kids(info->bmp, info->child);
      info = info->sibling;
   }

   _dispsw_status = SWITCH_NONE;

   if (bitmap_color_depth(screen) == 8) {
      if (_got_prev_current_palette)
	 gfx_driver->set_palette(_prev_current_palette, 0, 255, FALSE);
      else
	 gfx_driver->set_palette(_current_palette, 0, 255, FALSE);
   }

   if (hadmouse)
      _al_linker_mouse->show_mouse(screen);

   _timer_installed = hadtimer;
}


