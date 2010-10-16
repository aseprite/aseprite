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
 *      DirectDraw gfx drivers header.
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */


#ifndef WDDRAW_H_INCLUDED
#define WDDRAW_H_INCLUDED

#define DIRECTDRAW_VERSION 0x0300

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"

#ifndef SCAN_DEPEND
   #ifdef ALLEGRO_BCC32
      #undef NONAMELESSUNION
   #endif

   #include <ddraw.h>
#endif

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif


/* wrapper for DirectDraw surfaces */
typedef struct DDRAW_SURFACE {
   LPDIRECTDRAWSURFACE2 id;
   int flags;
   int lock_nesting;
   BITMAP *parent_bmp;  /* only used by the flipping chain */
   struct DDRAW_SURFACE *next;
   struct DDRAW_SURFACE *prev;
} DDRAW_SURFACE;


#define DDRAW_SURFACE_OF(bmp) ((DDRAW_SURFACE *)(bmp->extra))
#define DDRAW_SURFACE_PRIMARY   0x01
#define DDRAW_SURFACE_OVERLAY   0x02
#define DDRAW_SURFACE_VIDEO     0x04
#define DDRAW_SURFACE_SYSTEM    0x08
#define DDRAW_SURFACE_TYPE_MASK 0x0F
#define DDRAW_SURFACE_INDEXED   0x10
#define DDRAW_SURFACE_LOST      0x20


/* DirectDraw globals (from wddraw.c) */
AL_VAR(LPDIRECTDRAW2, directdraw);
AL_VAR(LPDIRECTDRAWCLIPPER, ddclipper);
AL_VAR(LPDIRECTDRAWPALETTE, ddpalette);
AL_VAR(LPDDPIXELFORMAT, ddpixel_format);
AL_VAR(DDCAPS, ddcaps);

AL_VAR(DDRAW_SURFACE *, gfx_directx_primary_surface);
AL_VAR(BITMAP *, gfx_directx_forefront_bitmap);
AL_VAR(unsigned char *, pseudo_surf_mem);


/* driver routines */
AL_FUNC(BITMAP *, gfx_directx_init, (GFX_DRIVER *drv, int w, int h, int v_w, int v_h, int color_depth));
AL_FUNC(void, gfx_directx_exit, (BITMAP *bmp));
AL_FUNC(void, gfx_directx_sync, (void));
AL_FUNC(void, gfx_directx_set_palette, (AL_CONST RGB *p, int from, int to, int vsync));
AL_FUNC(int, gfx_directx_poll_scroll, (void));
AL_FUNC(void, gfx_directx_created_sub_bitmap, (BITMAP *bmp, BITMAP *parent));
AL_FUNC(BITMAP *, gfx_directx_create_video_bitmap, (int width, int height));
AL_FUNC(void, gfx_directx_destroy_video_bitmap, (BITMAP *bmp));
AL_FUNC(int, gfx_directx_show_video_bitmap, (BITMAP *bmp));
AL_FUNC(int, gfx_directx_request_video_bitmap, (BITMAP *bmp));
AL_FUNC(BITMAP *, gfx_directx_create_system_bitmap, (int width, int height));
AL_FUNC(void, gfx_directx_destroy_system_bitmap, (BITMAP *bmp));
AL_FUNC(GFX_MODE_LIST *, gfx_directx_fetch_mode_list, (void));
AL_FUNC(int, gfx_directx_set_mouse_sprite, (struct BITMAP *sprite, int xfocus, int yfocus));
AL_FUNC(int, gfx_directx_show_mouse, (struct BITMAP *bmp, int x, int y));
AL_FUNC(void, gfx_directx_hide_mouse, (void));
AL_FUNC(void, gfx_directx_move_mouse, (int x, int y));


/* driver initialisation and shutdown (from wddraw.c) */
AL_FUNC(int, init_directx, (void));
AL_FUNC(int, gfx_directx_create_primary, (void));
AL_FUNC(int, gfx_directx_create_clipper, (HWND hwnd));
AL_FUNC(int, gfx_directx_create_palette, (DDRAW_SURFACE *surf));
AL_FUNC(int, gfx_directx_setup_driver, (GFX_DRIVER * drv, int w, int h, int color_depth));
AL_FUNC(int, finalize_directx_init, (void));
AL_FUNC(int, exit_directx, (void));


/* driver initialisation and shutdown (from wddaccel.c) */
AL_FUNC(void, gfx_directx_enable_acceleration, (GFX_DRIVER *drv));
AL_FUNC(void, gfx_directx_enable_triple_buffering, (GFX_DRIVER *drv));


/* video mode setting (from wddmode.c) */
AL_VAR(int, _win_desktop_depth);
AL_VAR(RGB_MAP, _win_desktop_rgb_map);

AL_FUNC(void, build_desktop_rgb_map, (void));
AL_FUNC(int, gfx_directx_compare_color_depth, (int color_depth));
AL_FUNC(int, gfx_directx_update_color_format, (DDRAW_SURFACE *surf, int color_depth));
AL_FUNC(int, set_video_mode, (int w, int h, int v_w, int v_h, int color_depth));


/* bitmap locking (from wddlock.c and wgdi.c and possibly asmlock.s) */
AL_FUNC(void, gfx_directx_lock, (BITMAP *bmp));
AL_FUNC(void, gfx_directx_autolock, (BITMAP* bmp));
AL_FUNC(void, gfx_directx_unlock, (BITMAP *bmp));
AL_FUNC(void, gfx_directx_unlock_win, (BITMAP *bmp));
AL_FUNC(void, gfx_directx_release_lock, (BITMAP * bmp));
AL_FUNC(uintptr_t, gfx_directx_write_bank, (BITMAP *bmp, int line));
AL_FUNC(void,      gfx_directx_unwrite_bank, (BITMAP *bmp));
AL_FUNC(uintptr_t, gfx_directx_write_bank_win, (BITMAP *bmp, int line));
AL_FUNC(void,      gfx_directx_unwrite_bank_win, (BITMAP *bmp));
AL_FUNC(void,      gfx_gdi_autolock, (struct BITMAP* bmp));
AL_FUNC(void,      gfx_gdi_unlock, (struct BITMAP* bmp));
AL_FUNC(uintptr_t, gfx_gdi_write_bank, (BITMAP *bmp, int line));
AL_FUNC(void,      gfx_gdi_unwrite_bank, (BITMAP *bmp));
/* dirty window updating (from wddwin.c, used in wddlock.c or asmlock.s) */
AL_VAR(char *,     _al_wd_dirty_lines);
AL_FUNCPTR(void,   _al_wd_update_window, (RECT *rect));


/* bitmap creation (from wddbmp.c) */
AL_FUNC(DDRAW_SURFACE *, gfx_directx_create_surface, (int w, int h, LPDDPIXELFORMAT pixel_format, int type));
AL_FUNC(void, gfx_directx_destroy_surface, (DDRAW_SURFACE *surf));
AL_FUNC(BITMAP *, gfx_directx_make_bitmap_from_surface, (DDRAW_SURFACE *surf, int w, int h, int id));


/* video bitmap list (from wddbmpl.c) */
AL_FUNC(void, register_ddraw_surface, (DDRAW_SURFACE *surf));
AL_FUNC(void, unregister_ddraw_surface, (DDRAW_SURFACE *surf));
AL_FUNC(void, unregister_all_ddraw_surfaces, (void));
AL_FUNC(int, restore_all_ddraw_surfaces, (void));


#endif

