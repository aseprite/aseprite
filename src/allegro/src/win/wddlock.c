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
 *      DirectDraw bitmap locking.
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */


#include "wddraw.h"

#define PREFIX_I                "al-wddlock INFO: "
#define PREFIX_W                "al-wddlock WARNING: "
#define PREFIX_E                "al-wddlock ERROR: "



/* If custom (asm) calling conversions are used, then the code in asmlock.s is
 * used instead.
 */
#if defined(ALLEGRO_NO_ASM)

static void update_dirty_lines(BITMAP *bmp)
{
   RECT rect;
   int i;

   /* The width is always the full bitmap width, because we have no 
    * mechanism for measuring the X range of the update.
    */
   rect.left = 0;
   rect.right = bmp->w;
   for (i = 0; i < bmp->h; i++) {
      if (_al_wd_dirty_lines[i]) {
         int j = i+1;
         rect.top = i;
         /* consecutive dirty lines are combined into ranges of Y values */
         while (_al_wd_dirty_lines[j])
            j++;
         rect.bottom = j;
         _al_wd_update_window(&rect);
         i = j+1;
      }
   }
}



uintptr_t gfx_directx_write_bank(BITMAP *bmp, int line)
{
   if (!(bmp->id & BMP_ID_LOCKED)) 
      gfx_directx_autolock(bmp);

   return (uintptr_t) bmp->line[line];
}



void gfx_directx_unwrite_bank(BITMAP *bmp)
{
   if (!(bmp->id & BMP_ID_AUTOLOCK))
      return;

   gfx_directx_unlock(bmp);
   bmp->id &= ~ BMP_ID_AUTOLOCK;
}



uintptr_t gfx_directx_write_bank_win(BITMAP *bmp, int line)
{
   _al_wd_dirty_lines[bmp->y_ofs+line] = 1;

   if (!(bmp->id & BMP_ID_LOCKED))
      gfx_directx_autolock(bmp);

   return (uintptr_t) bmp->line[line];
}



void gfx_directx_unwrite_bank_win(BITMAP *bmp)
{
   if (!(bmp->id & BMP_ID_AUTOLOCK))
      return;

   gfx_directx_unlock(bmp);
   bmp->id &= ~BMP_ID_AUTOLOCK;

   /* Update dirty lines: this is safe because autolocking is guaranteed to
    * be the only level of locking.  (Or at least, that's what it says in
    * asmlock.s)
    */
   update_dirty_lines(gfx_directx_forefront_bitmap);
}



void gfx_directx_unlock_win(BITMAP *bmp)
{
   gfx_directx_unlock(bmp);

   /* forefront_bitmap may still be locked in case of nested locking */
   if (!(gfx_directx_forefront_bitmap->id & BMP_ID_LOCKED))
      update_dirty_lines(gfx_directx_forefront_bitmap);
}



#else /* !defined(ALLEGRO_NO_ASM) */



/* asmlock.s requires these two variables */
void (*ptr_gfx_directx_autolock) (BITMAP* bmp) = gfx_directx_autolock;
void (*ptr_gfx_directx_unlock) (BITMAP* bmp) = gfx_directx_unlock;



#endif /* !defined(ALLEGRO_NO_ASM) */



/* gfx_directx_switch_out:
 *  Arranges for drawing requests to pause when we are in the background.
 */
static void gfx_directx_switch_out(void)
{
   _exit_gfx_critical();

   if (GFX_CRITICAL_RELEASED)
      _win_thread_switch_out();

   _enter_gfx_critical();
}



/* gfx_directx_lock:
 *  Locks the surface and prepares the lines array of the bitmap.
 */
void gfx_directx_lock(BITMAP *bmp)
{
   DDRAW_SURFACE *surf;
   BITMAP *parent;
   HRESULT hr;
   DDSURFACEDESC ddsurf_desc;
   int pitch;
   unsigned char *data;
   int y;

   if (bmp->id & BMP_ID_SUB) {
      /* if it's a sub-bitmap, start by locking our parent */
      parent = (BITMAP *)bmp->extra;
      gfx_directx_lock(parent);
      bmp->id |= BMP_ID_LOCKED;

      /* update the line array if our parent has moved */
      pitch = (long)parent->line[1] - (long)parent->line[0];
      data = parent->line[0] +
             (bmp->y_ofs - parent->y_ofs) * pitch +
             (bmp->x_ofs - parent->x_ofs) * BYTES_PER_PIXEL(bitmap_color_depth(bmp));

      if (data != bmp->line[0]) {
	 for (y = 0; y < bmp->h; y++) {
	    bmp->line[y] = data;
	    data += pitch;
	 }
      }
   }
   else {
      /* require exclusive ownership of the bitmap */
      _enter_gfx_critical();

      /* handle display switch */
      if (!_win_app_foreground)
         gfx_directx_switch_out();

      /* this is a real bitmap, so can be locked directly */
      surf = DDRAW_SURFACE_OF(bmp);
      surf->lock_nesting++;

      if (!(bmp->id & BMP_ID_LOCKED)) {
         /* try to lock surface */
	 bmp->id |= BMP_ID_LOCKED;
	 surf->flags &= ~DDRAW_SURFACE_LOST;

	 ddsurf_desc.dwSize = sizeof(DDSURFACEDESC);
	 ddsurf_desc.dwFlags = 0;

	 hr = IDirectDrawSurface2_Lock(surf->id, NULL, &ddsurf_desc,
                                    DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);

	 /* If the surface has been lost, try to restore all surfaces
	  * and, on success, try again to lock the surface.
	  */
	 if (hr == DDERR_SURFACELOST) {
	    if (restore_all_ddraw_surfaces() == 0) {
	       ddsurf_desc.dwSize = sizeof(DDSURFACEDESC);
	       ddsurf_desc.dwFlags = 0;

	       hr = IDirectDrawSurface2_Lock(surf->id, NULL, &ddsurf_desc,
                                          DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
	    }
	 }

	 if (FAILED(hr)) {
	    _TRACE(PREFIX_E "Can't lock surface (%x)\n", hr);

	    /* lock failed, use pseudo surface memory */
	    surf->flags |= DDRAW_SURFACE_LOST;
	    data = pseudo_surf_mem;
	    pitch = 0;
	 } 
	 else {
	    data = ddsurf_desc.lpSurface;
	    pitch = ddsurf_desc.lPitch;
	 }

	 /* prepare line array */
	 if (data != bmp->line[0]) {
	    for (y = 0; y < bmp->h; y++) {
	       bmp->line[y] = data;
	       data += pitch;
	    }
         }
      }
   }
}



/* gfx_directx_autolock:
 *  Locks the surface and prepares the lines array of the bitmap.
 *  This version is used directly by the bank switch functions, ie.
 *  it handles the autolocking mode, rather than being called directly.
 */
void gfx_directx_autolock(BITMAP *bmp)
{
   DDRAW_SURFACE *surf;
   BITMAP *parent;
   int pitch;
   unsigned char *data;
   int y;

   if (bmp->id & BMP_ID_SUB) {
      /* if it's a sub-bitmap, start by locking our parent */
      parent = (BITMAP *)bmp->extra;
      gfx_directx_autolock(parent);
      bmp->id |= BMP_ID_LOCKED;
      if (parent->id & BMP_ID_AUTOLOCK) {
	 bmp->id |= BMP_ID_AUTOLOCK;
	 parent->id &= ~BMP_ID_AUTOLOCK;
      }

      /* update the line array if our parent has moved */
      pitch = (long)parent->line[1] - (long)parent->line[0];
      data = parent->line[0] +
             (bmp->y_ofs - parent->y_ofs) * pitch +
             (bmp->x_ofs - parent->x_ofs) * BYTES_PER_PIXEL(bitmap_color_depth(bmp));

      if (data != bmp->line[0]) {
	 for (y = 0; y < bmp->h; y++) {
	    bmp->line[y] = data;
	    data += pitch;
	 }
      }
   }
   else {
      /* this is a real bitmap, so can be locked directly */
      surf = DDRAW_SURFACE_OF(bmp);

      if (surf->lock_nesting) {
	 /* re-locking after a hwaccel, so don't change nesting state */
	 gfx_directx_lock(bmp);
	 surf->lock_nesting--;
	 _exit_gfx_critical();
      }
      else {
	 /* locking for the first time */
	 gfx_directx_lock(bmp);
	 bmp->id |= BMP_ID_AUTOLOCK;
      }
   }
}



/* gfx_directx_unlock:
 *  Unlocks the surface.
 */
void gfx_directx_unlock(BITMAP *bmp)
{
   DDRAW_SURFACE *surf;
   BITMAP *parent;
   HRESULT hr;

   if (bmp->id & BMP_ID_SUB) {
      /* recurse when unlocking sub-bitmaps */
      parent = (BITMAP *)bmp->extra;
      gfx_directx_unlock(parent);
      if (!(parent->id & BMP_ID_LOCKED))
	 bmp->id &= ~BMP_ID_LOCKED;
   }
   else {
      /* regular bitmaps can be unlocked directly */
      surf = DDRAW_SURFACE_OF(bmp);

      if (surf->lock_nesting > 0) {
	 surf->lock_nesting--;

         if ((!surf->lock_nesting) && (bmp->id & BMP_ID_LOCKED)) {
            if (!(surf->flags & DDRAW_SURFACE_LOST)) {
	       /* only unlock if it doesn't use pseudo video memory */
	       hr = IDirectDrawSurface2_Unlock(surf->id, NULL);

	       /* If the surface has been lost, try to restore all surfaces
	        * and, on success, try again to unlock the surface.
	        */
	       if (hr == DDERR_SURFACELOST) {
	          if (restore_all_ddraw_surfaces() == 0)
	             hr = IDirectDrawSurface2_Unlock(surf->id, NULL);
	       }

	       if (FAILED(hr))
	          _TRACE(PREFIX_E "Can't unlock surface (%x)\n", hr);
            }

            bmp->id &= ~BMP_ID_LOCKED;
         }

         /* release bitmap for other threads */
         _exit_gfx_critical();
      }
   }
}



/* gfx_directx_release_lock:
 *  Releases the surface lock, for hardware accelerated drawing.
 */
void gfx_directx_release_lock(BITMAP *bmp)
{
   DDRAW_SURFACE *surf;
   HRESULT hr;

   /* handle display switch */
   if (!_win_app_foreground)
      gfx_directx_switch_out();

   /* find parent */
   while (bmp->id & BMP_ID_SUB) {
      bmp->id &= ~BMP_ID_LOCKED;
      bmp = (BITMAP *)bmp->extra;
   }

   if (bmp->id & BMP_ID_LOCKED) {
      surf = DDRAW_SURFACE_OF(bmp);

      if (!(surf->flags & DDRAW_SURFACE_LOST)) {
	 /* only unlock if it doesn't use pseudo video memory */
	 hr = IDirectDrawSurface2_Unlock(surf->id, NULL);

	 /* If the surface has been lost, try to restore all surfaces
	  * and, on success, try again to unlock the surface.
	  */
	 if (hr == DDERR_SURFACELOST) {
	    if (restore_all_ddraw_surfaces() == 0)
	       hr = IDirectDrawSurface2_Unlock(surf->id, NULL);
	 }

	 if (FAILED(hr))
	    _TRACE(PREFIX_E "Can't release lock (%x)\n", hr);
      }

      bmp->id &= ~BMP_ID_LOCKED;
   }
}
