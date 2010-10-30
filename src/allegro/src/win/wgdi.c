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
 *      Windowed mode gfx driver using gdi.
 *
 *      By Stefan Schimanski.
 *
 *      Dirty rectangles mechanism and hardware mouse cursor emulation
 *      by Eric Botcazou. 
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h" 
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif

#define PREFIX_I                "al-wgdi INFO: "
#define PREFIX_W                "al-wgdi WARNING: "
#define PREFIX_E                "al-wgdi ERROR: "


static void gfx_gdi_autolock(struct BITMAP* bmp);
static void gfx_gdi_unlock(struct BITMAP* bmp);

/* This is used only in asmlock.s and this file. */
char *gdi_dirty_lines = NULL; /* used in WRITE_BANK() */



uintptr_t gfx_gdi_write_bank(BITMAP *bmp, int line)
{
   gdi_dirty_lines[bmp->y_ofs +line] = 1;

   if (!(bmp->id & BMP_ID_LOCKED))
      gfx_gdi_autolock(bmp);

   return (uintptr_t) bmp->line[line];
}



void gfx_gdi_unwrite_bank(BITMAP *bmp)
{
   if (!(bmp->id & BMP_ID_AUTOLOCK))
      return;

   gfx_gdi_unlock(bmp);
   bmp->id &= ~ BMP_ID_AUTOLOCK;
}



static struct BITMAP *gfx_gdi_init(int w, int h, int v_w, int v_h, int color_depth);
static void gfx_gdi_exit(struct BITMAP *b);
static void gfx_gdi_set_palette(AL_CONST struct RGB *p, int from, int to, int vsync);
static void gfx_gdi_vsync(void);
/* hardware mouse cursor emulation */
static int  gfx_gdi_set_mouse_sprite(struct BITMAP *sprite, int xfocus, int yfocus);
static int  gfx_gdi_show_mouse(struct BITMAP *bmp, int x, int y);
static void gfx_gdi_hide_mouse(void);
static void gfx_gdi_move_mouse(int x, int y);
static BITMAP *gfx_gdi_acknowledge_resize(void);

static BITMAP *_create_gdi_screen(int w, int h, int color_depth);
static void _destroy_gdi_screen(void);



GFX_DRIVER gfx_gdi =
{
   GFX_GDI,
   empty_string,
   empty_string,
   "GDI",
   gfx_gdi_init,
   gfx_gdi_exit,
   NULL,                        // AL_METHOD(int, scroll, (int x, int y)); 
   gfx_gdi_vsync,
   gfx_gdi_set_palette,
   NULL,                        // AL_METHOD(int, request_scroll, (int x, int y));
   NULL,                        // AL_METHOD(int poll_scroll, (void));
   NULL,                        // AL_METHOD(void, enable_triple_buffer, (void));
   NULL, NULL, NULL, NULL, NULL, NULL,
   gfx_gdi_set_mouse_sprite,
   gfx_gdi_show_mouse,
   gfx_gdi_hide_mouse,
   gfx_gdi_move_mouse,
   NULL,                        // AL_METHOD(void, drawing_mode, (void));
   NULL,                        // AL_METHOD(void, save_video_state, (void*));
   NULL,                        // AL_METHOD(void, restore_video_state, (void*));
   NULL,                        // AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a));
   NULL,                        // AL_METHOD(int, fetch_mode_list, (void));
   gfx_gdi_acknowledge_resize,
   0, 0,                        // int w, h;
   TRUE,                        // int linear;
   0,                           // long bank_size;
   0,                           // long bank_gran;
   0,                           // long vid_mem;
   0,                           // long vid_phys_base;
   TRUE                         // int windowed;
};


static void gdi_enter_sysmode(void);
static void gdi_exit_sysmode(void);
static void gdi_update_window(RECT *rect);


static WIN_GFX_DRIVER win_gfx_driver_gdi =
{
   TRUE,
   NULL,                        // AL_METHOD(void, switch_in, (void));
   NULL,                        // AL_METHOD(void, switch_out, (void));
   gdi_enter_sysmode,
   gdi_exit_sysmode,
   NULL,                        // AL_METHOD(void, move, (int x, int y, int w, int h));
   NULL,                        // AL_METHOD(void, iconify, (void));
   gdi_update_window
};


static char *screen_surf;
static BITMAP *gdi_screen;
static int lock_nesting = 0;
static int render_semaphore = FALSE;
static PALETTE palette;
static HANDLE vsync_event;
#define RENDER_DELAY (1000/70)  /* 70 Hz */

/* hardware mouse cursor emulation */
static int mouse_on = FALSE;
static int mouse_was_on = FALSE;
static BITMAP *wgdi_mouse_sprite = NULL;
static BITMAP *mouse_frontbuffer = NULL;
static BITMAP *mouse_backbuffer = NULL;
static int mouse_xfocus, mouse_yfocus;
static int mouse_xpos, mouse_ypos;



/* gfx_gdi_set_mouse_sprite:
 */
static int gfx_gdi_set_mouse_sprite(struct BITMAP *sprite, int xfocus, int yfocus)
{
   if (wgdi_mouse_sprite) {
      destroy_bitmap(wgdi_mouse_sprite);
      wgdi_mouse_sprite = NULL;

      destroy_bitmap(mouse_frontbuffer);
      mouse_frontbuffer = NULL;

      destroy_bitmap(mouse_backbuffer);
      mouse_backbuffer = NULL;
   }

   wgdi_mouse_sprite = create_bitmap(sprite->w, sprite->h);
   blit(sprite, wgdi_mouse_sprite, 0, 0, 0, 0, sprite->w, sprite->h);

   mouse_xfocus = xfocus;
   mouse_yfocus = yfocus;

   mouse_frontbuffer = create_bitmap(sprite->w, sprite->h);
   mouse_backbuffer = create_bitmap(sprite->w, sprite->h);

   return 0;
}



/* update_mouse_pointer:
 *  Worker function that updates the mouse pointer.
 */
static void update_mouse_pointer(int x, int y, int retrace)
{
   HDC hdc;
   HWND allegro_wnd = win_get_window();

   /* put the screen contents located at the new position into the frontbuffer */
   blit(gdi_screen, mouse_frontbuffer, x, y, 0, 0, mouse_frontbuffer->w, mouse_frontbuffer->h);

   /* draw the mouse pointer onto the frontbuffer */
   draw_sprite(mouse_frontbuffer, wgdi_mouse_sprite, 0, 0);

   hdc = GetDC(allegro_wnd);

   if (_color_depth == 8)
      set_palette_to_hdc(hdc, palette);

   if (retrace) {
      /* restore the screen contents located at the old position */
      blit_to_hdc(mouse_backbuffer, hdc, 0, 0, mouse_xpos, mouse_ypos, mouse_backbuffer->w, mouse_backbuffer->h);
   }

   /* blit the mouse pointer onto the screen */
   blit_to_hdc(mouse_frontbuffer, hdc, 0, 0, x, y, mouse_frontbuffer->w, mouse_frontbuffer->h);

   ReleaseDC(allegro_wnd, hdc);

   /* save the screen contents located at the new position into the backbuffer */
   blit(gdi_screen, mouse_backbuffer, x, y, 0, 0, mouse_backbuffer->w, mouse_backbuffer->h);

   /* save the new position */
   mouse_xpos = x;
   mouse_ypos = y;
}



/* gfx_gdi_show_mouse:
 */
static int gfx_gdi_show_mouse(struct BITMAP *bmp, int x, int y)
{
   /* handle only the screen */
   if (bmp != gdi_screen)
      return -1;

   mouse_on = TRUE;

   x -= mouse_xfocus;
   y -= mouse_yfocus;

   update_mouse_pointer(x, y, FALSE);

   return 0;
} 



/* gfx_gdi_hide_mouse:
 */
static void gfx_gdi_hide_mouse(void)
{
   HDC hdc;
   HWND allegro_wnd = win_get_window();

   if (!mouse_on)
      return; 

   hdc = GetDC(allegro_wnd);

   if (_color_depth == 8)
      set_palette_to_hdc(hdc, palette);

   /* restore the screen contents located at the old position */
   blit_to_hdc(mouse_backbuffer, hdc, 0, 0, mouse_xpos, mouse_ypos, mouse_backbuffer->w, mouse_backbuffer->h);

   ReleaseDC(allegro_wnd, hdc);

   mouse_on = FALSE;
}

 

/* gfx_gdi_move_mouse:
 */
static void gfx_gdi_move_mouse(int x, int y)
{
   if (!mouse_on)
      return;

   x -= mouse_xfocus;
   y -= mouse_yfocus;

   if ((mouse_xpos == x) && (mouse_ypos == y))
      return;

   update_mouse_pointer(x, y, TRUE);
} 



/* render_proc:
 *  Timer proc that updates the window.
 */
static void render_proc(void)
{
   int top_line, bottom_line;
   HDC hdc = NULL;
   HWND allegro_wnd = win_get_window();

   /* to prevent reentrant calls */
   if (render_semaphore)
      return;

   render_semaphore = TRUE;

   /* to prevent the drawing threads and the rendering proc
    * from concurrently accessing the dirty lines array.
    */
   _enter_gfx_critical();

   if (!gdi_screen) {
      _exit_gfx_critical();
      render_semaphore = FALSE;
      return;
   }

   /* pseudo dirty rectangles mechanism:
    *  at most only one GDI call is performed for each frame,
    *  a true dirty rectangles mechanism makes the demo game
    *  unplayable in 640x480 on my system.
    */

   /* find the first dirty line */
   top_line = 0;

   while (!gdi_dirty_lines[top_line])
      top_line++;

   if (top_line < gfx_gdi.h) {
      /* find the last dirty line */
      bottom_line = gfx_gdi.h-1;

      while (!gdi_dirty_lines[bottom_line])
         bottom_line--;

      hdc = GetDC(allegro_wnd);

      if (_color_depth == 8)
         set_palette_to_hdc(hdc, palette);

      blit_to_hdc(gdi_screen, hdc, 0, top_line, 0, top_line,
                  gfx_gdi.w, bottom_line - top_line + 1);

      /* update mouse pointer if needed */
      if (mouse_on) {
         if ((mouse_ypos+wgdi_mouse_sprite->h > top_line) && (mouse_ypos <= bottom_line)) {
            blit(gdi_screen, mouse_backbuffer, mouse_xpos, mouse_ypos, 0, 0,
                 mouse_backbuffer->w, mouse_backbuffer->h);

            update_mouse_pointer(mouse_xpos, mouse_ypos, TRUE);
         }
      }
      
      /* clean up the dirty lines */
      while (top_line <= bottom_line)
         gdi_dirty_lines[top_line++] = 0;

      ReleaseDC(allegro_wnd, hdc);
   }

   _exit_gfx_critical();

   /* simulate vertical retrace */
   PulseEvent(vsync_event);

   render_semaphore = FALSE;
}



/* gdi_enter_sysmode:
 */
static void gdi_enter_sysmode(void)
{
   /* hide the mouse pointer */
   if (mouse_on) {
      mouse_was_on = TRUE;
      gfx_gdi_hide_mouse();
      _TRACE(PREFIX_I "mouse pointer off\n");
   }
}



/* gdi_exit_sysmode:
 */
static void gdi_exit_sysmode(void)
{
   if (mouse_was_on) {
      mouse_on = TRUE;
      mouse_was_on = FALSE;
      _TRACE(PREFIX_I "mouse pointer on\n");
   }
}



/* gdi_update_window:
 *  Updates the window.
 */
static void gdi_update_window(RECT *rect)
{
   HDC hdc;
   HWND allegro_wnd = win_get_window();

   _enter_gfx_critical();

   if (!gdi_screen) {
      _exit_gfx_critical();
      return;
   }

   hdc = GetDC(allegro_wnd);

   if (_color_depth == 8)
      set_palette_to_hdc(hdc, palette);

   blit_to_hdc(gdi_screen, hdc, rect->left, rect->top, rect->left, rect->top,
               rect->right - rect->left, rect->bottom - rect->top);

   ReleaseDC(allegro_wnd, hdc);

   _exit_gfx_critical();
}



/* gfx_gdi_lock:
 */
static void gfx_gdi_lock(struct BITMAP *bmp)
{
   /* to prevent the drawing threads and the rendering proc
    * from concurrently accessing the dirty lines array
    */
   _enter_gfx_critical();

   /* arrange for drawing requests to pause when we are in the background */
   if (!_win_app_foreground) {
      /* stop timer */
      remove_int(render_proc);

      _exit_gfx_critical();

      if (GFX_CRITICAL_RELEASED)
         _win_thread_switch_out();

      _enter_gfx_critical();

      /* restart timer */
      install_int(render_proc, RENDER_DELAY);
   }

   lock_nesting++;
   bmp->id |= BMP_ID_LOCKED;
}



/* gfx_gdi_autolock:
 */
static void gfx_gdi_autolock(struct BITMAP *bmp)
{
   gfx_gdi_lock(bmp);
   bmp->id |= BMP_ID_AUTOLOCK;
}

 

/* gfx_gdi_unlock:
 */
static void gfx_gdi_unlock(struct BITMAP *bmp)
{
   if (lock_nesting > 0) {
      lock_nesting--;

      if (!lock_nesting)
         bmp->id &= ~BMP_ID_LOCKED;

      _exit_gfx_critical();
   }
}



/* gfx_gdi_init:
 */
static struct BITMAP *gfx_gdi_init(int w, int h, int v_w, int v_h, int color_depth)
{
   /* virtual screen are not supported */
   if ((v_w!=0 && v_w!=w) || (v_h!=0 && v_h!=h))
      return NULL;
   
   _enter_critical();

   gfx_gdi.w = w;
   gfx_gdi.h = h;

   if (adjust_window(w, h) != 0) {
      _TRACE(PREFIX_E "window size not supported.\n");
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
      goto Error;
   }

   _create_gdi_screen(w, h, color_depth);

   /* create render timer */
   vsync_event = CreateEvent(NULL, FALSE, FALSE, NULL);
   install_int(render_proc, RENDER_DELAY);

   /* connect to the system driver */
   win_gfx_driver = &win_gfx_driver_gdi;

   /* set the default switching policy */
   set_display_switch_mode(SWITCH_PAUSE);

   _exit_critical();

   return gdi_screen;

 Error:
   _exit_critical();

   gfx_gdi_exit(NULL);

   return NULL;
}



/* gfx_gdi_exit:
 */
static void gfx_gdi_exit(struct BITMAP *bmp)
{
   _enter_critical();

   _enter_gfx_critical();

   if (bmp) {
      save_window_pos();
      clear_bitmap(bmp);
   }

   /* stop timer */
   remove_int(render_proc);
   CloseHandle(vsync_event);

   /* disconnect from the system driver */
   win_gfx_driver = NULL;

   _destroy_gdi_screen();

   /* destroy mouse bitmaps */
   if (wgdi_mouse_sprite) {
      destroy_bitmap(wgdi_mouse_sprite);
      wgdi_mouse_sprite = NULL;

      destroy_bitmap(mouse_frontbuffer);
      mouse_frontbuffer = NULL;

      destroy_bitmap(mouse_backbuffer);
      mouse_backbuffer = NULL;
   }

   _exit_gfx_critical();

   /* before restoring video mode, hide window */
   set_display_switch_mode(SWITCH_PAUSE);
   system_driver->restore_console_state();
   restore_window_style();

   _exit_critical();
}



/* gfx_gdi_set_palette:
 */
static void gfx_gdi_set_palette(AL_CONST struct RGB *p, int from, int to, int vsync)
{
   int c;

   for (c=from; c<=to; c++)
      palette[c] = p[c];

   /* invalidate the whole screen */
   _enter_gfx_critical();
   gdi_dirty_lines[0] = gdi_dirty_lines[gfx_gdi.h-1] = 1;
   _exit_gfx_critical();
}



/* gfx_gdi_vsync:
 */
static void gfx_gdi_vsync(void)
{
   WaitForSingleObject(vsync_event, INFINITE);
}



static BITMAP *gfx_gdi_acknowledge_resize(void)
{
   HWND allegro_wnd = win_get_window();
   int color_depth = bitmap_color_depth(screen);
   int w, h;
   RECT rc;
   BITMAP *new_screen;

   GetClientRect(allegro_wnd, &rc);
   w = rc.right;
   h = rc.bottom;
   if (w % 4)
      w -= (w % 4);

   _enter_gfx_critical();
   
   /* Re-create the screen */
   _destroy_gdi_screen();
   new_screen = _create_gdi_screen(w, h, color_depth);

   _exit_gfx_critical();

   return new_screen;
}



static BITMAP *_create_gdi_screen(int w, int h, int color_depth)
{
   gfx_gdi.w = w;
   gfx_gdi.h = h;

   /* the last flag serves as an end of loop delimiter */
   gdi_dirty_lines = _AL_MALLOC_ATOMIC((h+1) * sizeof(char));
   ASSERT(gdi_dirty_lines);
   memset(gdi_dirty_lines, 0, (h+1) * sizeof(char));
   gdi_dirty_lines[h] = 1;

   /* create the screen surface */
   screen_surf = _AL_MALLOC_ATOMIC(w * h * BYTES_PER_PIXEL(color_depth));
   gdi_screen = _make_bitmap(w, h, (unsigned long)screen_surf, &gfx_gdi, color_depth, w * BYTES_PER_PIXEL(color_depth));
   gdi_screen->write_bank = gfx_gdi_write_bank; 
   _screen_vtable.acquire = gfx_gdi_lock;
   _screen_vtable.release = gfx_gdi_unlock;
   _screen_vtable.unwrite_bank = gfx_gdi_unwrite_bank; 

   return gdi_screen;
}



static void _destroy_gdi_screen(void)
{
   /* destroy dirty lines array */   
   _AL_FREE(gdi_dirty_lines);
   gdi_dirty_lines = NULL;   

   /* destroy screen surface */
   _AL_FREE(screen_surf);
   gdi_screen = NULL;
}
