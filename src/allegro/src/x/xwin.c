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
 *      Wrappers for Xlib functions.
 *
 *      By Michael Bukin.
 *
 *      Video mode switching by Peter Wang and Benjamin Stover.
 *
 *      X icon selection by Evert Glebbeek
 *
 *      Added resize support by David Capello.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"
#include "xwin.h"

#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#ifdef ALLEGRO_XWINDOWS_WITH_SHM
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
#include <X11/extensions/xf86vmode.h>
#endif

#ifdef ALLEGRO_XWINDOWS_WITH_XCURSOR
#include <X11/Xcursor/Xcursor.h>
#endif


#ifdef ALLEGRO_XWINDOWS_WITH_XPM
#include <X11/xpm.h>
#endif
#include "icon.xpm"


#define XWIN_DEFAULT_WINDOW_TITLE "Allegro application"
#define XWIN_DEFAULT_APPLICATION_NAME "allegro"
#define XWIN_DEFAULT_APPLICATION_CLASS "Allegro"


struct _xwin_type _xwin =
{
   0,           /* display */
   0,           /* lock count */
   0,           /* screen */
   None,        /* window */
   None,        /* gc */
   0,           /* visual */
   None,        /* colormap */
   0,           /* ximage */
#ifdef ALLEGRO_XWINDOWS_WITH_XCURSOR
   None,        /* ARGB cursor image */
   XcursorFalse,/* Are ARGB cursors supported? */
#endif
   None,        /* cursor */
   XC_heart,    /* cursor_shape */
   1,           /* hw_cursor_ok */

   0,           /* screen_to_buffer */
   0,           /* set_colors */

   0,           /* screen_data */
   0,           /* screen_line */
   0,           /* buffer_line */

   0,           /* scroll_x */
   0,           /* scroll_y */

   320,         /* window_width */
   200,         /* window_height */
   8,           /* window_depth */

   320,         /* screen_width */
   200,         /* screen_height */
   8,           /* screen_depth */

   320,         /* virtual width */
   200,         /* virtual_height */

   0,           /* mouse_warped */
   { 0 },       /* keycode_to_scancode */

   0,           /* matching formats */
   0,           /* fast_visual_depth */
   0,           /* visual_is_truecolor */

   1,           /* rsize */
   1,           /* gsize */
   1,           /* bsize */
   0,           /* rshift */
   0,           /* gshift */
   0,           /* bshift */

   { 0 },       /* cmap */
   { 0 },       /* rmap */
   { 0 },       /* gmap */
   { 0 },       /* bmap */

#ifdef ALLEGRO_XWINDOWS_WITH_SHM
   { 0, 0, 0, 0 },  /* shminfo */
#endif
   0,           /* use_shm */

   0,           /* in_dga_mode */

   0,           /* keyboard_grabbed */
   0,           /* mouse_grabbed */

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
   0,           /* modesinfo */
   0,           /* num_modes */
   0,           /* mode_switched */
   0,           /* override_redirected */
#endif

   XWIN_DEFAULT_WINDOW_TITLE,           /* window_title */
   XWIN_DEFAULT_APPLICATION_NAME,       /* application_name */
   XWIN_DEFAULT_APPLICATION_CLASS,      /* application_class */

   FALSE,       /* drawing_mode_ok */

#ifdef ALLEGRO_MULTITHREADED
   NULL,        /* mutex */
#endif

   NULL,        /* window close hook */
   NULL,        /* window resize hook */
#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
   0,           /* orig_modeinfo */
#endif
   None,        /* fs_window */
   None         /* wm_window */
};

void *allegro_icon = icon_xpm;

int _xwin_last_line = -1;
int _xwin_in_gfx_call = 0;

static COLORCONV_BLITTER_FUNC *blitter_func = NULL;
static int use_bgr_palette_hack = FALSE; /* use BGR hack for color conversion palette? */

#ifndef ALLEGRO_MULTITHREADED
int _xwin_missed_input;
#endif

#define X_MAX_EVENTS       50
#define MOUSE_WARP_DELAY   200

static char _xwin_driver_desc[256] = EMPTY_STRING;

/* This is used to intercept window closing requests.  */
static Atom wm_delete_window;

#define PREFIX_I                "al-xwin INFO: "
#define PREFIX_W                "al-xwin WARNING: "
#define PREFIX_E                "al-xwin ERROR: "


/* Forward declarations for private functions.  */
static int _xwin_private_open_display(char *name);
static int _xwin_private_create_window(void);
static void _xwin_private_destroy_window(void);
static void _xwin_private_select_screen_to_buffer_function(void);
static void _xwin_private_select_set_colors_function(void);
static void _xwin_private_setup_driver_desc(GFX_DRIVER *drv);
static BITMAP *_xwin_private_create_screen(GFX_DRIVER *drv, int w, int h,
                                           int vw, int vh, int depth, int fullscreen);
static void _xwin_private_destroy_screen(void);
static void _xwin_private_destroy_screen_data(void);
static BITMAP *_xwin_private_create_screen_data(GFX_DRIVER *drv, int w, int h);
static BITMAP *_xwin_private_create_screen_bitmap(GFX_DRIVER *drv,
                                                  unsigned char *frame_buffer,
                                                  int bytes_per_buffer_line);
static void _xwin_private_create_mapping_tables(void);
static void _xwin_private_create_mapping(unsigned long *map, int ssize, int dsize, int dshift);
static int _xwin_private_display_is_local(void);
static int _xwin_private_create_ximage(int w, int h);
static void _xwin_private_destroy_ximage(void);
static void _xwin_private_prepare_visual(void);
static int _xwin_private_matching_formats(void);
static void _xwin_private_hack_shifts(void);
static int _xwin_private_colorconv_usable(void);
static int _xwin_private_fast_visual_depth(void);
static void _xwin_private_set_matching_colors(AL_CONST PALETTE p, int from, int to);
static void _xwin_private_set_truecolor_colors(AL_CONST PALETTE p, int from, int to);
static void _xwin_private_set_palette_colors(AL_CONST PALETTE p, int from, int to);
static void _xwin_private_set_palette_range(AL_CONST PALETTE p, int from, int to);
static void _xwin_private_set_window_defaults(void);
static void _xwin_private_flush_buffers(void);
static void _xwin_private_process_event(XEvent *event);
static void _xwin_private_set_warped_mouse_mode(int permanent);
static void _xwin_private_redraw_window(int x, int y, int w, int h);
static int _xwin_private_scroll_screen(int x, int y);
static void _xwin_private_update_screen(int x, int y, int w, int h);
static void _xwin_private_set_window_title(AL_CONST char *name);
static void _xwin_private_set_window_name(AL_CONST char *name, AL_CONST char *group);
static int _xwin_private_get_pointer_mapping(unsigned char map[], int nmap);

static void _xwin_private_fast_colorconv(int sx, int sy, int sw, int sh);

static void _xwin_private_fast_truecolor_8_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_8_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_8_to_24(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_8_to_32(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_15_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_15_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_15_to_24(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_15_to_32(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_16_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_16_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_16_to_24(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_16_to_32(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_24_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_24_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_24_to_24(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_24_to_32(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_32_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_32_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_32_to_24(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_32_to_32(int sx, int sy, int sw, int sh);

static void _xwin_private_slow_truecolor_8(int sx, int sy, int sw, int sh);
static void _xwin_private_slow_truecolor_15(int sx, int sy, int sw, int sh);
static void _xwin_private_slow_truecolor_16(int sx, int sy, int sw, int sh);
static void _xwin_private_slow_truecolor_24(int sx, int sy, int sw, int sh);
static void _xwin_private_slow_truecolor_32(int sx, int sy, int sw, int sh);

static void _xwin_private_fast_palette_8_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_8_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_8_to_32(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_15_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_15_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_15_to_32(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_16_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_16_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_16_to_32(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_24_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_24_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_24_to_32(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_32_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_32_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_32_to_32(int sx, int sy, int sw, int sh);

static void _xwin_private_slow_palette_8(int sx, int sy, int sw, int sh);
static void _xwin_private_slow_palette_15(int sx, int sy, int sw, int sh);
static void _xwin_private_slow_palette_16(int sx, int sy, int sw, int sh);
static void _xwin_private_slow_palette_24(int sx, int sy, int sw, int sh);
static void _xwin_private_slow_palette_32(int sx, int sy, int sw, int sh);

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
static void _xvidmode_private_set_fullscreen(int w, int h, int *vidmode_width,
   int *vidmode_height);
static void _xvidmode_private_unset_fullscreen(void);
#endif

uintptr_t _xwin_write_line(BITMAP *bmp, int line);
void _xwin_unwrite_line(BITMAP *bmp);



/* _xwin_open_display:
 *  Wrapper for XOpenDisplay.
 */
static int _xwin_private_open_display(char *name)
{
   if (_xwin.display != 0)
      return -1;

   _xwin.display = XOpenDisplay(name);
   _xwin.screen = ((_xwin.display == 0) ? 0 : XDefaultScreen(_xwin.display));

   return ((_xwin.display != 0) ? 0 : -1);
}

int _xwin_open_display(char *name)
{
   int result;
   XLOCK();
   result = _xwin_private_open_display(name);
   XUNLOCK();
   return result;
}



/* _xwin_close_display:
 *  Wrapper for XCloseDisplay.
 */
void _xwin_close_display(void)
{
   Display *dpy;

   if (!_unix_bg_man->multi_threaded) {
      XLOCK();
   }

   if (_xwin.display != 0) {
      _xwin_destroy_window();
      dpy = _xwin.display;
      _xwin.display = 0;
      XCloseDisplay(dpy);
   }

   if (!_unix_bg_man->multi_threaded) {
      XUNLOCK();
   }
}



/* _xwin_hide_x_mouse:
 *  Create invisible X cursor.
 */
static void _xwin_hide_x_mouse(void)
{
   unsigned long gcmask;
   XGCValues gcvalues;
   Pixmap pixmap;

   XUndefineCursor(_xwin.display, _xwin.window);

   if (_xwin.cursor != None) {
      XFreeCursor(_xwin.display, _xwin.cursor);
      _xwin.cursor = None;
   }

#ifdef ALLEGRO_XWINDOWS_WITH_XCURSOR
   if (_xwin.xcursor_image != None) {
      XcursorImageDestroy(_xwin.xcursor_image);
      _xwin.xcursor_image = None;
   }
#endif

   pixmap = XCreatePixmap(_xwin.display, _xwin.window, 1, 1, 1);
   if (pixmap != None) {
      GC temp_gc;
      XColor color;

      gcmask = GCFunction | GCForeground | GCBackground;
      gcvalues.function = GXcopy;
      gcvalues.foreground = 0;
      gcvalues.background = 0;
      temp_gc = XCreateGC(_xwin.display, pixmap, gcmask, &gcvalues);
      XDrawPoint(_xwin.display, pixmap, temp_gc, 0, 0);
      XFreeGC(_xwin.display, temp_gc);
      color.pixel = 0;
      color.red = color.green = color.blue = 0;
      color.flags = DoRed | DoGreen | DoBlue;
      _xwin.cursor = XCreatePixmapCursor(_xwin.display, pixmap, pixmap, &color, &color, 0, 0);
      XDefineCursor(_xwin.display, _xwin.window, _xwin.cursor);
      XFreePixmap(_xwin.display, pixmap);
   }
   else {
      _xwin.cursor = XCreateFontCursor(_xwin.display, _xwin.cursor_shape);
      XDefineCursor(_xwin.display, _xwin.window, _xwin.cursor);
   }
}



/* _xwin_wait_mapped:
 *  Wait for a window to become mapped.
 */
static void _xwin_wait_mapped(Window win)
{
   /* Note:
    * The busy loop below is just a hack to work around my broken X11.
    * A call to XMaskEvent will block indefinitely and no Allegro
    * programs (which create a window) will start up at all. Replacing
    * XMaskEvent with a busy loop calling XCheckMaskEvent repeatedly
    * somehow works though..
    */
   while (1) {
       XEvent e;
       if (XCheckTypedEvent(_xwin.display, MapNotify, &e)) {
          if (e.xmap.event == win) break;
       }
       rest(1);
   }
}



/* _xwin_create_window:
 *  We use 3 windows:
 *  - fs_window (for fullscreen)
 *  - wm_window (window managed)
 *  - window    (the real window)
 *
 *  Two of which will be created here: wm_window and window. The fullscreen
 *  window gets (re)created when needed, because reusing it causes trouble see
 *  http://sourceforge.net/tracker/index.php?func=detail&aid=1441740&group_id=5665&atid=105665
 *  The real window uses wm_window as parent initially and will be reparented
 *  to the (freshly created) fullscreen window when requested and reparented
 *  back again in screen_destroy.
 *
 *  Idea/concept of three windows borrowed from SDL. But somehow SDL manages
 *  to reuse the fullscreen window too.
 */
static int _xwin_private_create_window(void)
{
   unsigned long gcmask;
   XGCValues gcvalues;
   XSetWindowAttributes setattr;
   XWindowAttributes getattr;

   if (_xwin.display == 0)
      return -1;

   _mouse_on = FALSE;

   /* Create the managed window. */
   setattr.background_pixel = XBlackPixel(_xwin.display, _xwin.screen);
   setattr.border_pixel = XBlackPixel(_xwin.display, _xwin.screen);
   setattr.event_mask = (KeyPressMask | KeyReleaseMask | StructureNotifyMask
                         | EnterWindowMask | LeaveWindowMask
                         | FocusChangeMask | ExposureMask | PropertyChangeMask
                         | ButtonPressMask | ButtonReleaseMask | PointerMotionMask
                         /*| MappingNotifyMask (SubstructureRedirectMask?)*/);
   _xwin.wm_window = XCreateWindow(_xwin.display,
                                XDefaultRootWindow(_xwin.display),
                                0, 0, 320, 200, 0,
                                CopyFromParent, InputOutput, CopyFromParent,
                                CWBackPixel | CWBorderPixel | CWEventMask,
                                &setattr);

   /* Get associated visual and window depth (bits per pixel).  */
   XGetWindowAttributes(_xwin.display, _xwin.wm_window, &getattr);
   _xwin.visual = getattr.visual;
   _xwin.window_depth = getattr.depth;

   /* Create and install colormap.  */
   if ((_xwin.visual->class == PseudoColor)
       || (_xwin.visual->class == GrayScale)
       || (_xwin.visual->class == DirectColor))
   {
      _xwin.colormap = XCreateColormap(_xwin.display, _xwin.wm_window, _xwin.visual, AllocAll);
   }
   else {
      _xwin.colormap = XCreateColormap(_xwin.display, _xwin.wm_window, _xwin.visual, AllocNone);
   }
   XSetWindowColormap(_xwin.display, _xwin.wm_window, _xwin.colormap);
   XInstallColormap(_xwin.display, _xwin.colormap);

   /* Create the real / drawing window (reuses setattr). */
   setattr.colormap = _xwin.colormap;
   _xwin.window = XCreateWindow(_xwin.display,
                                _xwin.wm_window,
                                0, 0, 320, 200, 0,
                                CopyFromParent, InputOutput, CopyFromParent,
                                CWBackPixel | CWBorderPixel | CWEventMask |
                                CWColormap, &setattr);

   /* Map the real / drawing window it won't appear untill the parent does */
   XMapWindow(_xwin.display, _xwin.window);

   /* Set WM_DELETE_WINDOW atom in WM_PROTOCOLS property (to get window_delete requests).  */
   wm_delete_window = XInternAtom(_xwin.display, "WM_DELETE_WINDOW", False);
   XSetWMProtocols(_xwin.display, _xwin.wm_window, &wm_delete_window, 1);

   /* Set default window parameters.  */
   (*_xwin_window_defaultor)();

   /* Create graphics context.  */
   gcmask = GCFunction | GCForeground | GCBackground | GCFillStyle | GCPlaneMask;
   gcvalues.function = GXcopy;
   gcvalues.foreground = setattr.border_pixel;
   gcvalues.background = setattr.border_pixel;
   gcvalues.fill_style = FillSolid;
   gcvalues.plane_mask = AllPlanes;
   _xwin.gc = XCreateGC(_xwin.display, _xwin.window, gcmask, &gcvalues);

   /* Create invisible X cursor.  */
   _xwin_hide_x_mouse();

#ifdef ALLEGRO_XWINDOWS_WITH_XCURSOR
   /* Detect if ARGB cursors are supported */
   _xwin.support_argb_cursor = XcursorSupportsARGB(_xwin.display);
#endif
   _xwin.hw_cursor_ok = 0;

   return 0;
}

int _xwin_create_window(void)
{
   int result;
   XLOCK();
   result = (*_xwin_window_creator)();
   XUNLOCK();
   return result;
}



/* _xwin_destroy_window:
 *  Wrapper for XDestroyWindow.
 */
static void _xwin_private_destroy_window(void)
{
   _xwin_private_destroy_screen();

   if (_xwin.cursor != None) {
      XUndefineCursor(_xwin.display, _xwin.window);
      XFreeCursor(_xwin.display, _xwin.cursor);
      _xwin.cursor = None;
   }

#ifdef ALLEGRO_XWINDOWS_WITH_XCURSOR
   if (_xwin.xcursor_image != None) {
      XcursorImageDestroy(_xwin.xcursor_image);
      _xwin.xcursor_image = None;
   }
#endif

   _xwin.visual = 0;

   if (_xwin.gc != None) {
      XFreeGC(_xwin.display, _xwin.gc);
      _xwin.gc = None;
   }

   if (_xwin.colormap != None) {
      XUninstallColormap(_xwin.display, _xwin.colormap);
      XFreeColormap(_xwin.display, _xwin.colormap);
      _xwin.colormap = None;
   }

   if (_xwin.window != None) {
      XUnmapWindow(_xwin.display, _xwin.window);
      XDestroyWindow(_xwin.display, _xwin.window);
      _xwin.window = None;
   }

   if (_xwin.wm_window != None) {
      XDestroyWindow(_xwin.display, _xwin.wm_window);
      _xwin.wm_window = None;
   }
}

void _xwin_destroy_window(void)
{
   XLOCK();
   _xwin_private_destroy_window();
   XUNLOCK();
}



typedef void (*_XWIN_SCREEN_TO_BUFFER)(int x, int y, int w, int h);
static _XWIN_SCREEN_TO_BUFFER _xwin_screen_to_buffer_function[5][10] =
{
   {
      _xwin_private_slow_truecolor_8,
      _xwin_private_fast_truecolor_8_to_8,
      _xwin_private_fast_truecolor_8_to_16,
      _xwin_private_fast_truecolor_8_to_24,
      _xwin_private_fast_truecolor_8_to_32,
      _xwin_private_slow_palette_8,
      _xwin_private_fast_palette_8_to_8,
      _xwin_private_fast_palette_8_to_16,
      0,
      _xwin_private_fast_palette_8_to_32
   },
   {
      _xwin_private_slow_truecolor_15,
      _xwin_private_fast_truecolor_15_to_8,
      _xwin_private_fast_truecolor_15_to_16,
      _xwin_private_fast_truecolor_15_to_24,
      _xwin_private_fast_truecolor_15_to_32,
      _xwin_private_slow_palette_15,
      _xwin_private_fast_palette_15_to_8,
      _xwin_private_fast_palette_15_to_16,
      0,
      _xwin_private_fast_palette_15_to_32
   },
   {
      _xwin_private_slow_truecolor_16,
      _xwin_private_fast_truecolor_16_to_8,
      _xwin_private_fast_truecolor_16_to_16,
      _xwin_private_fast_truecolor_16_to_24,
      _xwin_private_fast_truecolor_16_to_32,
      _xwin_private_slow_palette_16,
      _xwin_private_fast_palette_16_to_8,
      _xwin_private_fast_palette_16_to_16,
      0,
      _xwin_private_fast_palette_16_to_32
   },
   {
      _xwin_private_slow_truecolor_24,
      _xwin_private_fast_truecolor_24_to_8,
      _xwin_private_fast_truecolor_24_to_16,
      _xwin_private_fast_truecolor_24_to_24,
      _xwin_private_fast_truecolor_24_to_32,
      _xwin_private_slow_palette_24,
      _xwin_private_fast_palette_24_to_8,
      _xwin_private_fast_palette_24_to_16,
      0,
      _xwin_private_fast_palette_24_to_32
   },
   {
      _xwin_private_slow_truecolor_32,
      _xwin_private_fast_truecolor_32_to_8,
      _xwin_private_fast_truecolor_32_to_16,
      _xwin_private_fast_truecolor_32_to_24,
      _xwin_private_fast_truecolor_32_to_32,
      _xwin_private_slow_palette_32,
      _xwin_private_fast_palette_32_to_8,
      _xwin_private_fast_palette_32_to_16,
      0,
      _xwin_private_fast_palette_32_to_32
   },
};



/* _xwin_select_screen_to_buffer_function:
 *  Select which function should be used for updating frame buffer with screen data.
 */
static void _xwin_private_select_screen_to_buffer_function(void)
{
   int i, j;

   if (_xwin.matching_formats) {
      _xwin.screen_to_buffer = 0;
   }
   else {
      switch (_xwin.screen_depth) {
         case 8: i = 0; break;
         case 15: i = 1; break;
         case 16: i = 2; break;
         case 24: i = 3; break;
         case 32: i = 4; break;
         default: i = 0; break;
      }
      switch (_xwin.fast_visual_depth) {
         case 0: j = 0; break;
         case 8: j = 1; break;
         case 16: j = 2; break;
         case 24: j = 3; break;
         case 32: j = 4; break;
         default: j = 0; break;
      }
      if (!_xwin.visual_is_truecolor)
         j += 5;

      if (_xwin_private_colorconv_usable()) {
         TRACE(PREFIX_I "Using generic color conversion blitter (%u, %u).\n",
               _xwin.screen_depth, _xwin.fast_visual_depth);
         blitter_func = _get_colorconv_blitter(_xwin.screen_depth,
                                               _xwin.fast_visual_depth);
         _xwin.screen_to_buffer = _xwin_private_fast_colorconv;
      }
      else {
         _xwin.screen_to_buffer = _xwin_screen_to_buffer_function[i][j];
      }
   }
}



/* _xwin_select_set_colors_function:
 *  Select which function should be used for setting hardware colors.
 */
static void _xwin_private_select_set_colors_function(void)
{
   if (_xwin.screen_depth != 8) {
      _xwin.set_colors = 0;
   }
   else {
      if (_xwin.matching_formats) {
         _xwin.set_colors = _xwin_private_set_matching_colors;
      }
      else if (_xwin.visual_is_truecolor) {
         _xwin.set_colors = _xwin_private_set_truecolor_colors;
      }
      else {
         _xwin.set_colors = _xwin_private_set_palette_colors;
      }
   }
}



/* _xwin_setup_driver_desc:
 *  Sets up the X-Windows driver description string.
 */
static void _xwin_private_setup_driver_desc(GFX_DRIVER *drv)
{
   char tmp1[256], tmp2[128], tmp3[128], tmp4[128];

   /* Prepare driver description.  */
   if (_xwin.matching_formats) {
      uszprintf(_xwin_driver_desc, sizeof(_xwin_driver_desc),
                uconvert_ascii("X-Windows graphics, in matching, %d bpp %s", tmp1),
                _xwin.window_depth,
                uconvert_ascii("real depth", tmp2));
   }
   else {
      uszprintf(_xwin_driver_desc, sizeof(_xwin_driver_desc),
                uconvert_ascii("X-Windows graphics, in %s %s, %d bpp %s", tmp1),
                uconvert_ascii((_xwin.fast_visual_depth ? "fast" : "slow"), tmp2),
                uconvert_ascii((_xwin.visual_is_truecolor ? "truecolor" : "paletted"), tmp3),
                _xwin.window_depth,
                uconvert_ascii("real depth", tmp4));
   }
   drv->desc = _xwin_driver_desc;
}



/* _xwin_create_screen:
 *  Creates screen data and other resources.
 */
static BITMAP *_xwin_private_create_screen(GFX_DRIVER *drv, int w, int h,
                                           int vw, int vh, int depth, int fullscreen)
{
   if (_xwin.window == None) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("No window"));
      return 0;
   }

   /* Choose convenient size.  */
   if ((w == 0) && (h == 0)) {
      w = 320;
      h = 200;
   }

   if (vw < w)
      vw = w;
   if (vh < h)
      vh = h;

   if (1
#ifdef ALLEGRO_COLOR8
       && (depth != 8)
#endif
#ifdef ALLEGRO_COLOR16
       && (depth != 15)
       && (depth != 16)
#endif
#ifdef ALLEGRO_COLOR24
       && (depth != 24)
#endif
#ifdef ALLEGRO_COLOR32
       && (depth != 32)
#endif
       ) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported color depth"));
      return 0;
   }

   /* Save dimensions.  */
   _xwin.window_width = w;
   _xwin.window_height = h;
   _xwin.screen_width = w;
   _xwin.screen_height = h;
   _xwin.screen_depth = depth;
   _xwin.virtual_width = vw;
   _xwin.virtual_height = vh;

   /* Resize the (real) window */
   XResizeWindow(_xwin.display, _xwin.window, w, h);

   if (fullscreen) {
      XSetWindowAttributes setattr;
      /* Local width and height vars used for fullscreen window size and for
       * storing the video_mode size which is then used to center the window.
       */
      int fs_width  = DisplayWidth(_xwin.display, _xwin.screen);
      int fs_height = DisplayHeight(_xwin.display, _xwin.screen);

      /* Create the fullscreen window.  */
      setattr.override_redirect = True;
      setattr.background_pixel = XBlackPixel(_xwin.display, _xwin.screen);
      setattr.border_pixel = XBlackPixel(_xwin.display, _xwin.screen);
      setattr.event_mask = StructureNotifyMask;
      setattr.colormap = _xwin.colormap;
      _xwin.fs_window = XCreateWindow(_xwin.display,
                                   XDefaultRootWindow(_xwin.display),
                                   0, 0, fs_width, fs_height, 0,
                                   CopyFromParent, InputOutput,
                                   CopyFromParent, CWOverrideRedirect |
                                   CWBackPixel | CWColormap | CWBorderPixel |
                                   CWEventMask, &setattr);

      /* Map the fullscreen window.  */
      XMapRaised(_xwin.display, _xwin.fs_window);
      _xwin_wait_mapped(_xwin.fs_window);

      /* Make sure we got to the top of the window stack.  */
      XRaiseWindow(_xwin.display, _xwin.fs_window);

      /* Reparent the real window.  */
      XReparentWindow(_xwin.display, _xwin.window, _xwin.fs_window, 0, 0);

      /* Grab the keyboard and mouse.  */
      if (XGrabKeyboard(_xwin.display, XDefaultRootWindow(_xwin.display), False,
                        GrabModeAsync, GrabModeAsync, CurrentTime) != GrabSuccess) {
         ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not grab keyboard"));
         return 0;
      }
      _xwin.keyboard_grabbed = 1;
      if (XGrabPointer(_xwin.display, _xwin.window, False,
                       PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
                       GrabModeAsync, GrabModeAsync, _xwin.window, None, CurrentTime) != GrabSuccess) {
         ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not grab mouse"));
         return 0;
      }
      _xwin.mouse_grabbed = 1;

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
      /* Try to switch video mode. This must be done after the pointer is
       * grabbed, because otherwise it can be outside the window negating the
       * XF86VidModeSetViewPort done in set_fullscreen. This makes the old
       * center the window hack unnescesarry. Notice that since the XF86VM
       * extension requests do not go through the regular X output buffer? We
       * need to make sure that all above requests are processed first.
       */
      XSync(_xwin.display, False);
      _xvidmode_private_set_fullscreen(w, h, &fs_width, &fs_height);
#endif

      /* Center the window (if necessary).  */
      if ((fs_width != w) || (fs_height != h))
         XMoveWindow(_xwin.display, _xwin.window, (fs_width - w) / 2,
            (fs_height - h) / 2);

      /* Last: center the cursor.  */
      XWarpPointer(_xwin.display, None, _xwin.window, 0, 0, 0, 0, w / 2, h / 2);
   }
   else {
      /* Resize managed window.  */
      XResizeWindow(_xwin.display, _xwin.wm_window, w, h);

      /* Map the window managed window.  */
      XMapWindow(_xwin.display, _xwin.wm_window);
      _xwin_wait_mapped(_xwin.wm_window);
   }

   return _xwin_private_create_screen_data(drv, vw, vh);
}

BITMAP *_xwin_create_screen(GFX_DRIVER *drv, int w, int h,
                            int vw, int vh, int depth, int fullscreen)
{
   BITMAP *bmp;
   XLOCK();
   bmp = _xwin_private_create_screen(drv, w, h, vw, vh, depth, fullscreen);
   if (bmp == 0) {
      _xwin_private_destroy_screen();
   }
   XUNLOCK();
   return bmp;
}



/* _xwin_destroy_screen:
 *  Destroys screen resources.
 */
static void _xwin_private_destroy_screen(void)
{
   _xwin_private_destroy_screen_data();

   if (_xwin.mouse_grabbed) {
      XUngrabPointer(_xwin.display, CurrentTime);
      _xwin.mouse_grabbed = 0;
   }

   if (_xwin.keyboard_grabbed) {
      XUngrabKeyboard(_xwin.display, CurrentTime);
      _xwin.keyboard_grabbed = 0;
   }

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
   _xvidmode_private_unset_fullscreen();
#endif

   /* whack color-conversion blitter */
   if (blitter_func) {
      _release_colorconv_blitter(blitter_func);
      blitter_func = NULL;
   }

   if (_xwin.fs_window != None) {
      /* Reparent the real window! */
      XReparentWindow(_xwin.display, _xwin.window, _xwin.wm_window, 0, 0);
      XUnmapWindow(_xwin.display, _xwin.fs_window);
      XDestroyWindow(_xwin.display, _xwin.fs_window);
      _xwin.fs_window = None;
   }
   else {
      XUnmapWindow(_xwin.display, _xwin.wm_window);
   }

   (*_xwin_window_defaultor)();
}

static void _xwin_private_destroy_screen_data(void)
{
   if (_xwin.buffer_line != 0) {
      _AL_FREE(_xwin.buffer_line);
      _xwin.buffer_line = 0;
   }

   if (_xwin.screen_line != 0) {
      _AL_FREE(_xwin.screen_line);
      _xwin.screen_line = 0;
   }

   if (_xwin.screen_data != 0) {
      _AL_FREE(_xwin.screen_data);
      _xwin.screen_data = 0;
   }

   _xwin_private_destroy_ximage();
}

void _xwin_destroy_screen(void)
{
   XLOCK();
   _xwin_private_destroy_screen();
   XUNLOCK();
}



static BITMAP *_xwin_private_rebuild_screen(int w, int h, int color_depth)
{
   _xwin_private_destroy_screen_data();
   destroy_bitmap(screen);

   /* Save dimensions.  */
   _xwin.window_width = w;
   _xwin.window_height = h;
   _xwin.screen_width = w;
   _xwin.screen_height = h;
   _xwin.screen_depth = color_depth;
   _xwin.virtual_width = w;
   _xwin.virtual_height = h;

   /* Resize the (real) window */
   XResizeWindow(_xwin.display, _xwin.window, w, h);

   return _xwin_private_create_screen_data(gfx_driver, w, h);
}

BITMAP *_xwin_rebuild_screen(int w, int h, int color_depth)
{
   BITMAP *new_screen;

   XLOCK();
   new_screen = _xwin_private_rebuild_screen(w, h, color_depth);
   XUNLOCK();

   return new_screen;
}



static BITMAP *_xwin_private_create_screen_data(GFX_DRIVER *drv, int w, int h)
{
   /* Create XImage with the size of virtual screen.  */
   if (_xwin_private_create_ximage(w, h) != 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not create XImage"));
      return NULL;
   }

   /* Prepare visual for further use.  */
   _xwin_private_prepare_visual();

   /* Test that frame buffer is fast (can be accessed directly).  */
   _xwin.fast_visual_depth = _xwin_private_fast_visual_depth();

   /* Create screen bitmap from frame buffer.  */
   return _xwin_private_create_screen_bitmap(drv,
                                             (unsigned char *)_xwin.ximage->data + _xwin.ximage->xoffset,
                                             _xwin.ximage->bytes_per_line);
}


/* _xwin_create_screen_bitmap:
 *  Create screen bitmap from frame buffer.
 */
static BITMAP *_xwin_private_create_screen_bitmap(GFX_DRIVER *drv,
                                                  unsigned char *frame_buffer,
                                                  int bytes_per_buffer_line)
{
   int line;
   int bytes_per_screen_line;
   BITMAP *bmp;

   /* Test that Allegro and X-Windows pixel formats are the same.  */
   _xwin.matching_formats = _xwin_private_matching_formats();

   /* Create mapping tables for color components.  */
   _xwin_private_create_mapping_tables();

   /* Determine how to update frame buffer with screen data.  */
   _xwin_private_select_screen_to_buffer_function();

   /* Determine how to set colors in "hardware".  */
   _xwin_private_select_set_colors_function();

   /* Create line accelerators for screen data.  */
   _xwin.screen_line = _AL_MALLOC_ATOMIC(_xwin.virtual_height * sizeof(unsigned char*));
   if (_xwin.screen_line == 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
      return 0;
   }

   /* If formats match, then use frame buffer as screen data, otherwise malloc.  */
   if (_xwin.matching_formats) {
      bytes_per_screen_line = bytes_per_buffer_line;
      _xwin.screen_data = 0;
      _xwin.screen_line[0] = frame_buffer;
   }
   else {
      bytes_per_screen_line = _xwin.virtual_width * BYTES_PER_PIXEL(_xwin.screen_depth);
      _xwin.screen_data = _AL_MALLOC_ATOMIC(_xwin.virtual_height * bytes_per_screen_line);
      if (_xwin.screen_data == 0) {
         ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
         return 0;
      }
      _xwin.screen_line[0] = _xwin.screen_data;
   }

   /* Initialize line starts.  */
   for (line = 1; line < _xwin.virtual_height; line++)
      _xwin.screen_line[line] = _xwin.screen_line[line - 1] + bytes_per_screen_line;

   /* Create line accelerators for frame buffer.  */
   if (!_xwin.matching_formats && _xwin.fast_visual_depth) {
      _xwin.buffer_line = _AL_MALLOC(_xwin.virtual_height * sizeof(unsigned char*));
      if (_xwin.buffer_line == 0) {
         ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
         return 0;
      }

      _xwin.buffer_line[0] = frame_buffer;
      for (line = 1; line < _xwin.virtual_height; line++)
         _xwin.buffer_line[line] = _xwin.buffer_line[line - 1] + bytes_per_buffer_line;
   }

   /* Create bitmap.  */
   bmp = _make_bitmap(_xwin.virtual_width, _xwin.virtual_height,
                      (uintptr_t) (_xwin.screen_line[0]), drv,
                      _xwin.screen_depth, bytes_per_screen_line);
   if (bmp == 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
      return 0;
   }

   /* Fixup bitmap fields.  */
   drv->w = bmp->cr = _xwin.screen_width;
   drv->h = bmp->cb = _xwin.screen_height;
   drv->vid_mem = _xwin.virtual_width * _xwin.virtual_height * BYTES_PER_PIXEL(_xwin.screen_depth);

   /* Need some magic for updating frame buffer.  */
   bmp->write_bank = _xwin_write_line;
   bmp->vtable->unwrite_bank = _xwin_unwrite_line;

   /* Replace entries in vtable with magical wrappers.  */
   _xwin_replace_vtable(bmp->vtable);

   /* The drawing mode may not be ok for direct updates anymore. */
   _xwin_drawing_mode();

   /* Initialize other fields in _xwin structure.  */
   _xwin_last_line = -1;
   _xwin_in_gfx_call = 0;
   _xwin.scroll_x = 0;
   _xwin.scroll_y = 0;

   /* Setup driver description string.  */
   _xwin_private_setup_driver_desc(drv);

   return bmp;
}



/* _xwin_create_mapping_tables:
 *  Create mapping between Allegro color component and X-Windows color component.
 */
static void _xwin_private_create_mapping_tables(void)
{
   if (!_xwin.matching_formats) {
      if (_xwin.visual_is_truecolor) {
         switch (_xwin.screen_depth) {
            case 8:
               /* Will be modified later in set_palette.  */
               _xwin_private_create_mapping(_xwin.rmap, 256, 0, 0);
               _xwin_private_create_mapping(_xwin.gmap, 256, 0, 0);
               _xwin_private_create_mapping(_xwin.bmap, 256, 0, 0);
               break;
            case 15:
               _xwin_private_create_mapping(_xwin.rmap, 32, _xwin.rsize, _xwin.rshift);
               _xwin_private_create_mapping(_xwin.gmap, 32, _xwin.gsize, _xwin.gshift);
               _xwin_private_create_mapping(_xwin.bmap, 32, _xwin.bsize, _xwin.bshift);
               break;
            case 16:
               _xwin_private_create_mapping(_xwin.rmap, 32, _xwin.rsize, _xwin.rshift);
               _xwin_private_create_mapping(_xwin.gmap, 64, _xwin.gsize, _xwin.gshift);
               _xwin_private_create_mapping(_xwin.bmap, 32, _xwin.bsize, _xwin.bshift);
               break;
            case 24:
            case 32:
               _xwin_private_create_mapping(_xwin.rmap, 256, _xwin.rsize, _xwin.rshift);
               _xwin_private_create_mapping(_xwin.gmap, 256, _xwin.gsize, _xwin.gshift);
               _xwin_private_create_mapping(_xwin.bmap, 256, _xwin.bsize, _xwin.bshift);
               break;
         }
      }
      else {
         int i;

         /* Might be modified later in set_palette.  */
         for (i = 0; i < 256; i++)
            _xwin.rmap[i] = _xwin.gmap[i] = _xwin.bmap[i] = 0;
      }
   }
}



/* _xwin_create_mapping:
 *  Create mapping between Allegro color component and X-Windows color component.
 */
static void _xwin_private_create_mapping(unsigned long *map, int ssize, int dsize, int dshift)
{
   int i, smax, dmax;

   smax = ssize - 1;
   dmax = dsize - 1;
   for (i = 0; i < ssize; i++)
      map[i] = ((dmax * i) / smax) << dshift;
   for (; i < 256; i++)
      map[i] = map[i % ssize];
}



/* _xwin_display_is_local:
 *  Tests that display connection is local.
 *  (Note: this is duplicated in xdga2.c).
 */
static int _xwin_private_display_is_local(void)
{
   char *name;

   if (_xwin.display == 0)
      return 0;

   /* Get display name and test for local display.  */
   name = XDisplayName(0);

   return (((name == 0) || (name[0] == ':') || (strncmp(name, "unix:", 5) == 0)) ? 1 : 0);
}



/* _xwin_create_ximage:
 *  Create XImage for accessing window.
 */
static int _xwin_private_create_ximage(int w, int h)
{
   XImage *image = 0; /* I'm mage or I'm old?  */

   if (_xwin.display == 0)
      return -1;

#ifdef ALLEGRO_XWINDOWS_WITH_SHM
   if (_xwin_private_display_is_local() && XShmQueryExtension(_xwin.display))
      _xwin.use_shm = 1;
   else
      _xwin.use_shm = 0;
#else
   _xwin.use_shm = 0;
#endif

#ifdef ALLEGRO_XWINDOWS_WITH_SHM
   if (_xwin.use_shm) {
      /* Try to create shared memory XImage.  */
      image = XShmCreateImage(_xwin.display, _xwin.visual, _xwin.window_depth,
                              ZPixmap, 0, &_xwin.shminfo, w, h);
      do {
         if (image != 0) {
            /* Create shared memory segment.  */
            _xwin.shminfo.shmid = shmget(IPC_PRIVATE, image->bytes_per_line * image->height,
                                         IPC_CREAT | 0777);
            if (_xwin.shminfo.shmid != -1) {
               /* Attach shared memory to our address space.  */
               _xwin.shminfo.shmaddr = image->data = shmat(_xwin.shminfo.shmid, 0, 0);
               if (_xwin.shminfo.shmaddr != (char*) -1) {
                  _xwin.shminfo.readOnly = True;

                  /* Attach shared memory to the X-server address space.  */
                  if (XShmAttach(_xwin.display, &_xwin.shminfo)) {
                     XSync(_xwin.display, False);
                     break;
                   }

                  shmdt(_xwin.shminfo.shmaddr);
               }
               shmctl(_xwin.shminfo.shmid, IPC_RMID, 0);
            }
            XDestroyImage(image);
            image = 0;
         }
         _xwin.use_shm = 0;
      } while (0);
   }
#endif

   if (image == 0) {
      /* Try to create ordinary XImage.  */
#if 0
      Pixmap pixmap;

      pixmap = XCreatePixmap(_xwin.display, _xwin.window, w, h, _xwin.window_depth);
      if (pixmap != None) {
         image = XGetImage(_xwin.display, pixmap, 0, 0, w, h, AllPlanes, ZPixmap);
         XFreePixmap(_xwin.display, pixmap);
      }
#else
      image = XCreateImage(_xwin.display, _xwin.visual, _xwin.window_depth,
                           ZPixmap, 0, 0, w, h, 32, 0);
      if (image != 0) {
         image->data = _AL_MALLOC_ATOMIC(image->bytes_per_line * image->height);
         if (image->data == 0) {
            XDestroyImage(image);
            image = 0;
         }
      }
#endif
   }

   _xwin.ximage = image;

   return ((image != 0) ? 0 : -1);
}



/* _xwin_destroy_ximage:
 *  Destroy XImage.
 */
static void _xwin_private_destroy_ximage(void)
{
   if (_xwin.ximage != 0) {
#ifdef ALLEGRO_XWINDOWS_WITH_SHM
      if (_xwin.use_shm) {
         XShmDetach(_xwin.display, &_xwin.shminfo);
         shmdt(_xwin.shminfo.shmaddr);
         shmctl(_xwin.shminfo.shmid, IPC_RMID, 0);
      }
#endif
      XDestroyImage(_xwin.ximage);
      _xwin.ximage = 0;
   }
}



/* _xwin_prepare_visual:
 *  Prepare visual for further use.
 */
static void _xwin_private_prepare_visual(void)
{
   int i, r, g, b;
   int rmax, gmax, bmax;
   unsigned long mask;
   XColor color;

   if ((_xwin.visual->class == TrueColor)
       || (_xwin.visual->class == DirectColor)) {
      /* Use TrueColor and DirectColor visuals as truecolor.  */

      /* Red shift and size.  */
      for (mask = _xwin.visual->red_mask, i = 0; (mask & 1) != 1; mask >>= 1)
         i++;
      _xwin.rshift = i;
      for (i = 0; mask != 0; mask >>= 1)
         i++;
      _xwin.rsize = 1 << i;

      /* Green shift and size.  */
      for (mask = _xwin.visual->green_mask, i = 0; (mask & 1) != 1; mask >>= 1)
         i++;
      _xwin.gshift = i;
      for (i = 0; mask != 0; mask >>= 1)
         i++;
      _xwin.gsize = 1 << i;

      /* Blue shift and size.  */
      for (mask = _xwin.visual->blue_mask, i = 0; (mask & 1) != 1; mask >>= 1)
         i++;
      _xwin.bshift = i;
      for (i = 0; mask != 0; mask >>= 1)
         i++;
      _xwin.bsize = 1 << i;

      /* Convert DirectColor visual into true color visual.  */
      if (_xwin.visual->class == DirectColor) {
         color.flags = DoRed;
         rmax = _xwin.rsize - 1;
         gmax = _xwin.gsize - 1;
         bmax = _xwin.bsize - 1;
         for (i = 0; i < _xwin.rsize; i++) {
            color.pixel = i << _xwin.rshift;
            color.red = ((rmax <= 0) ? 0 : ((i * 65535L) / rmax));
            XStoreColor(_xwin.display, _xwin.colormap, &color);
         }
         color.flags = DoGreen;
         for (i = 0; i < _xwin.gsize; i++) {
            color.pixel = i << _xwin.gshift;
            color.green = ((gmax <= 0) ? 0 : ((i * 65535L) / gmax));
            XStoreColor(_xwin.display, _xwin.colormap, &color);
         }
         color.flags = DoBlue;
         for (i = 0; i < _xwin.bsize; i++) {
            color.pixel = i << _xwin.bshift;
            color.blue = ((bmax <= 0) ? 0 : ((i * 65535L) / bmax));
            XStoreColor(_xwin.display, _xwin.colormap, &color);
         }
      }

      _xwin.visual_is_truecolor = 1;
   }
   else if ((_xwin.visual->class == PseudoColor)
            || (_xwin.visual->class == GrayScale)) {
      /* Convert read-write palette visual into true color visual.  */
      b = _xwin.window_depth / 3;
      r = (_xwin.window_depth - b) / 2;
      g = _xwin.window_depth - r - b;

      _xwin.rsize = 1 << r;
      _xwin.gsize = 1 << g;
      _xwin.bsize = 1 << b;
      _xwin.rshift = g + b;
      _xwin.gshift = b;
      _xwin.bshift = 0;

      _xwin.visual_is_truecolor = 1;

      rmax = _xwin.rsize - 1;
      gmax = _xwin.gsize - 1;
      bmax = _xwin.bsize - 1;

      color.flags = DoRed | DoGreen | DoBlue;
      for (r = 0; r < _xwin.rsize; r++) {
         for (g = 0; g < _xwin.gsize; g++) {
            for (b = 0; b < _xwin.bsize; b++) {
               color.pixel = (r << _xwin.rshift) | (g << _xwin.gshift) | (b << _xwin.bshift);
               color.red = ((rmax <= 0) ? 0 : ((r * 65535L) / rmax));
               color.green = ((gmax <= 0) ? 0 : ((g * 65535L) / gmax));
               color.blue = ((bmax <= 0) ? 0 : ((b * 65535L) / bmax));
               XStoreColor(_xwin.display, _xwin.colormap, &color);
            }
         }
      }
   }
   else {
      /* All other visual types are paletted.  */
      _xwin.rsize = 1;
      _xwin.bsize = 1;
      _xwin.gsize = 1;
      _xwin.rshift = 0;
      _xwin.gshift = 0;
      _xwin.bshift = 0;

      _xwin.visual_is_truecolor = 0;

      /* Make fixed palette and create mapping RRRRGGGGBBBB -> palette index.  */
      for (r = 0; r < 16; r++) {
         for (g = 0; g < 16; g++) {
            for (b = 0; b < 16; b++) {
               color.red = (r * 65535L) / 15;
               color.green = (g * 65535L) / 15;
               color.blue = (b * 65535L) / 15;
               XAllocColor(_xwin.display, _xwin.colormap, &color);
               _xwin.cmap[(r << 8) | (g << 4) | b] = color.pixel;
            }
         }
      }
   }
}



/* _xwin_matching_formats:
 *  Find if color formats match.
 */
static int _xwin_private_matching_formats(void)
{
   if (_xwin.fast_visual_depth == 0)
      return 0;

   if (_xwin.screen_depth == 8) {
      /* For matching 8 bpp modes visual must be PseudoColor or GrayScale.  */
      if (((_xwin.visual->class != PseudoColor)
           && (_xwin.visual->class != GrayScale))
          || (_xwin.fast_visual_depth != 8)
          || (_xwin.window_depth != 8))
         return 0;
   }
   else if ((_xwin.visual->class != TrueColor)
            && (_xwin.visual->class != DirectColor)) {
      /* For matching true color modes visual must be TrueColor or DirectColor.  */
      return 0;
   }
   else if (_xwin.screen_depth == 15) {
      if ((_xwin.fast_visual_depth != 16)
          || (_xwin.rsize != 32) || (_xwin.gsize != 32) || (_xwin.bsize != 32)
          || ((_xwin.rshift != 0) && (_xwin.rshift != 10))
          || ((_xwin.bshift != 0) && (_xwin.bshift != 10))
          || (_xwin.gshift != 5))
         return 0;
      _rgb_r_shift_15 = _xwin.rshift;
      _rgb_g_shift_15 = _xwin.gshift;
      _rgb_b_shift_15 = _xwin.bshift;
   }
   else if (_xwin.screen_depth == 16) {
      if ((_xwin.fast_visual_depth != 16)
          || (_xwin.rsize != 32) || (_xwin.gsize != 64) || (_xwin.bsize != 32)
          || ((_xwin.rshift != 0) && (_xwin.rshift != 11))
          || ((_xwin.bshift != 0) && (_xwin.bshift != 11))
          || (_xwin.gshift != 5))
         return 0;
      _rgb_r_shift_16 = _xwin.rshift;
      _rgb_g_shift_16 = _xwin.gshift;
      _rgb_b_shift_16 = _xwin.bshift;
   }
   else if (_xwin.screen_depth == 24){
      if ((_xwin.fast_visual_depth != 24)
          || (_xwin.rsize != 256) || (_xwin.gsize != 256) || (_xwin.bsize != 256)
          || ((_xwin.rshift != 0) && (_xwin.rshift != 16))
          || ((_xwin.bshift != 0) && (_xwin.bshift != 16))
          || (_xwin.gshift != 8))
         return 0;
      _rgb_r_shift_24 = _xwin.rshift;
      _rgb_g_shift_24 = _xwin.gshift;
      _rgb_b_shift_24 = _xwin.bshift;
   }
   else if (_xwin.screen_depth == 32) {
      if ((_xwin.fast_visual_depth != 32)
          || (_xwin.rsize != 256) || (_xwin.gsize != 256) || (_xwin.bsize != 256)
          || ((_xwin.rshift != 0) && (_xwin.rshift != 16))
          || ((_xwin.bshift != 0) && (_xwin.bshift != 16))
          || (_xwin.gshift != 8))
         return 0;
      _rgb_r_shift_32 = _xwin.rshift;
      _rgb_g_shift_32 = _xwin.gshift;
      _rgb_b_shift_32 = _xwin.bshift;
   }
   else {
      /* How did you get in here?  */
      return 0;
   }

   return 1;
}



/* _xwin_private_hack_shifts:
 *  Make Allegro draw in BGR format if X uses BGR instead of RGB.
 */
static void _xwin_private_hack_shifts(void)
{
   switch(_xwin.screen_depth) {
      case 8:
         use_bgr_palette_hack = TRUE;
         break;
      case 15:
         _rgb_r_shift_15 = 10;
         _rgb_g_shift_15 = 5;
         _rgb_b_shift_15 = 0;
         break;
      case 16:
         _rgb_r_shift_16 = 11;
         _rgb_g_shift_16 = 5;
         _rgb_b_shift_16 = 0;
         break;
      case 24:
         _rgb_r_shift_24 = 16;
         _rgb_g_shift_24 = 8;
         _rgb_b_shift_24 = 0;
         break;
      case 32:
         _rgb_r_shift_32 = 16;
         _rgb_g_shift_32 = 8;
         _rgb_b_shift_32 = 0;
         break;
   }
}



/* _xwin_private_colorconv_usable:
 *  Find out if generic color conversion blitter can be used.
 */
static int _xwin_private_colorconv_usable(void)
{
   use_bgr_palette_hack = FALSE; /* make sure this is initialized */

   if (_xwin.fast_visual_depth == 0) {
      return 0;
   }
   else if (_xwin.fast_visual_depth == 8) {
#if 0
      /* TODO: check this out later. */

      /* For usable 8 bpp modes visual must be PseudoColor or GrayScale.  */
      if ((_xwin.visual->class == PseudoColor)
           || (_xwin.visual->class == GrayScale))
         return 1;
#else
      return 0;
#endif
   }
   else if ((_xwin.visual->class != TrueColor)
            && (_xwin.visual->class != DirectColor)) {
      /* For usable true color modes visual must be TrueColor or DirectColor.  */
      return 0;
   }
   else if ((_xwin.fast_visual_depth == 16)
       && (_xwin.rsize == 32) && ((_xwin.gsize == 32) || (_xwin.gsize == 64)) && (_xwin.bsize == 32)
       && ((_xwin.rshift == 0) || (_xwin.rshift == 10) || (_xwin.rshift == 11))
       && ((_xwin.bshift == 0) || (_xwin.bshift == 10) || (_xwin.bshift == 11))
       && (_xwin.gshift == 5)) {
      if (_xwin.bshift == 0)
         _xwin_private_hack_shifts();
      return 1;
   }
   else if ((_xwin.fast_visual_depth == 24)
       && (_xwin.rsize == 256) && (_xwin.gsize == 256) && (_xwin.bsize == 256)
       && ((_xwin.rshift == 0) || (_xwin.rshift == 16))
       && ((_xwin.bshift == 0) || (_xwin.bshift == 16))
       && (_xwin.gshift == 8)) {
      if (_xwin.bshift == 0)
         _xwin_private_hack_shifts();
      return 1;
   }
   else if ((_xwin.fast_visual_depth == 32)
       && (_xwin.rsize == 256) && (_xwin.gsize == 256) && (_xwin.bsize == 256)
       && ((_xwin.rshift == 0) || (_xwin.rshift == 16))
       && ((_xwin.bshift == 0) || (_xwin.bshift == 16))
       && (_xwin.gshift == 8)) {
      if (_xwin.bshift == 0)
         _xwin_private_hack_shifts();
      return 1;
   }

   /* Too bad... Muahahaha! */
   return 0;
}



/* _xwin_fast_visual_depth:
 *  Find which depth is fast (when XImage can be accessed directly).
 */
static int _xwin_private_fast_visual_depth(void)
{
   int ok, x, sizex;
   int test_depth;
   uint8_t *p8;
   uint16_t *p16;
   uint32_t *p32;

   if (_xwin.ximage == 0)
      return 0;

   /* Use first line of XImage for test.  */
   p8 = (uint8_t *) _xwin.ximage->data + _xwin.ximage->xoffset;
   p16 = (uint16_t*) p8;
   p32 = (uint32_t*) p8;

   sizex = _xwin.ximage->bytes_per_line - _xwin.ximage->xoffset;

   if ((_xwin.window_depth < 1) || (_xwin.window_depth > 32)) {
      return 0;
   }
   else if (_xwin.window_depth > 16) {
      test_depth = 32;
      sizex /= sizeof (uint32_t);
   }
   else if (_xwin.window_depth > 8) {
      test_depth = 16;
      sizex /= sizeof (uint16_t);
   }
   else {
      test_depth = 8;
   }
   if (sizex > _xwin.ximage->width)
      sizex = _xwin.ximage->width;

   /* Need at least two pixels wide line for test.  */
   if (sizex < 2)
      return 0;

   ok = 1;
   for (x = 0; x < sizex; x++) {
      int bit;

      for (bit = -1; bit < _xwin.window_depth; bit++) {
         unsigned long color = ((bit < 0) ? 0 : ((unsigned long) 1 << bit));

         /* Write color through XImage API.  */
         XPutPixel(_xwin.ximage, x, 0, color);

         /* Read color with direct access.  */
         switch (test_depth) {
            case 8:
               if (p8[x] != color)
                  ok = 0;
               break;
            case 16:
               if (p16[x] != color)
                  ok = 0;
               break;
            case 32:
               if (p32[x] != color)
                  ok = 0;
               break;
            default:
               ok = 0;
               break;
         }
         XPutPixel(_xwin.ximage, x, 0, 0);

         if (!ok)
            return 0;
      }
   }

   return test_depth;
}



/* _xwin_enable_hardware_cursor:
 *  enable the hardware cursor; this disables the mouse mickey warping hack
 */
void _xwin_enable_hardware_cursor(int mode)
{
#ifdef ALLEGRO_XWINDOWS_WITH_XCURSOR
   if (_xwin.support_argb_cursor) {
      _xwin.hw_cursor_ok = mode;
   }
   else
#endif
   {
      _xwin.hw_cursor_ok = 0;
   }

   /* Switch to non-warped mode */
   if (_xwin.hw_cursor_ok) {
      _xwin.mouse_warped = 0;
      /* Move X-cursor to Allegro cursor.  */
      XLOCK();
      XWarpPointer(_xwin.display, _xwin.window, _xwin.window,
                   0, 0, _xwin.window_width, _xwin.window_height,
                   _mouse_x - (_xwin_mouse_extended_range ? _xwin.scroll_x : 0),
                   _mouse_y - (_xwin_mouse_extended_range ? _xwin.scroll_y : 0));
      XUNLOCK();
   }
}



#ifdef ALLEGRO_XWINDOWS_WITH_XCURSOR

/* _xwin_set_mouse_sprite:
 *  Set custom X cursor (if supported).
 */
int _xwin_set_mouse_sprite(struct BITMAP *sprite, int xfocus, int yfocus)
{
#define GET_PIXEL_DATA(depth, getpix)                                \
               case depth:                                           \
                  c = 0;                                             \
                  for (iy = 0; iy < sprite->h; iy++) {               \
                     for(ix = 0; ix < sprite->w; ix++) {             \
                        col = getpix(sprite, ix, iy);                \
                        if (col == (MASK_COLOR_ ## depth)) {         \
                           r = g = b = a = 0;                        \
                        }                                            \
                        else {                                       \
                           r = getr ## depth(col);                   \
                           g = getg ## depth(col);                   \
                           b = getb ## depth(col);                   \
                           a = 255;                                  \
                        }                                            \
                        _xwin.xcursor_image->pixels[c++] =           \
                                    (a<<24)|(r<<16)|(g<<8)|(b);      \
                     }                                               \
                  }

   if (!_xwin.support_argb_cursor) {
      return -1;
   }

   if (_xwin.xcursor_image != None) {
      XLOCK();
      XcursorImageDestroy(_xwin.xcursor_image);
      XUNLOCK();
      _xwin.xcursor_image = None;
   }

   if (sprite) {
      int ix, iy;
      int r = 0, g = 0, b = 0, a = 0, c, col;

      _xwin.xcursor_image = XcursorImageCreate(sprite->w, sprite->h);
      if (_xwin.xcursor_image == None) {
         return -1;
      }

      switch (bitmap_color_depth(sprite)) {
         GET_PIXEL_DATA(8, _getpixel)
            break;

         GET_PIXEL_DATA(15, _getpixel15)
            break;

         GET_PIXEL_DATA(16, _getpixel16)
            break;

         GET_PIXEL_DATA(24, _getpixel24)
            break;

         GET_PIXEL_DATA(32, _getpixel32)
            break;
      } /* End switch */

      _xwin.xcursor_image->xhot = xfocus;
      _xwin.xcursor_image->yhot = yfocus;

      return 0;
   }

   return -1;

#undef GET_PIXEL_DATA
}



/* _xwin_show_mouse:
 *  Show the custom X cursor (if supported)
 */
int _xwin_show_mouse(struct BITMAP *bmp, int x, int y)
{
   /* Only draw on screen */
   if (!is_same_bitmap(bmp, screen))
      return -1;

   if (!_xwin.support_argb_cursor) {
      return -1;
   }

   if (_xwin.xcursor_image == None) {
      return -1;
   }

   /* Hardware cursor is disabled (mickey mode) */
   if (!_xwin.hw_cursor_ok) {
      return -1;
   }

   XLOCK();
   if (_xwin.cursor != None) {
      XUndefineCursor(_xwin.display, _xwin.window);
      XFreeCursor(_xwin.display, _xwin.cursor);
   }

   _xwin.cursor = XcursorImageLoadCursor(_xwin.display, _xwin.xcursor_image);
   XDefineCursor(_xwin.display, _xwin.window, _xwin.cursor);

   XUNLOCK();
   return 0;
}



/* _xwin_hide_mouse:
 *  Hide the custom X cursor (if supported)
 */
void _xwin_hide_mouse(void)
{
   if (_xwin.support_argb_cursor) {
      XLOCK();
      _xwin_hide_x_mouse();
      XUNLOCK();
   }
   return;
}



/* _xwin_move_mouse:
 *  Get mouse move notification. Not that we need it...
 */
void _xwin_move_mouse(int x, int y)
{
}

#endif   /* ALLEGRO_XWINDOWS_WITH_XCURSOR */



/*
 * Functions for copying screen data to frame buffer.
 */
static void _xwin_private_fast_colorconv(int sx, int sy, int sw, int sh)
{
   GRAPHICS_RECT src_rect, dest_rect;

   /* set up source and destination rectangles */
   src_rect.height = sh;
   src_rect.width  = sw;
   src_rect.pitch  = _xwin.screen_line[1] - _xwin.screen_line[0];
   src_rect.data   = _xwin.screen_line[sy] + sx * BYTES_PER_PIXEL(_xwin.screen_depth);

   dest_rect.height = sh;
   dest_rect.width  = sw;
   dest_rect.pitch  = _xwin.buffer_line[1] - _xwin.buffer_line[0];
   dest_rect.data   = _xwin.buffer_line[sy] + sx * BYTES_PER_PIXEL(_xwin.fast_visual_depth);

   /* Update frame buffer with screen contents.  */
   ASSERT(blitter_func);
   blitter_func(&src_rect, &dest_rect);
}

#ifdef ALLEGRO_LITTLE_ENDIAN
   #define DEFAULT_RGB_R_POS_24  (DEFAULT_RGB_R_SHIFT_24/8)
   #define DEFAULT_RGB_G_POS_24  (DEFAULT_RGB_G_SHIFT_24/8)
   #define DEFAULT_RGB_B_POS_24  (DEFAULT_RGB_B_SHIFT_24/8)
#elif defined ALLEGRO_BIG_ENDIAN
   #define DEFAULT_RGB_R_POS_24  (2-DEFAULT_RGB_R_SHIFT_24/8)
   #define DEFAULT_RGB_G_POS_24  (2-DEFAULT_RGB_G_SHIFT_24/8)
   #define DEFAULT_RGB_B_POS_24  (2-DEFAULT_RGB_B_SHIFT_24/8)
#else
   #error endianess not defined
#endif

#define MAKE_FAST_TRUECOLOR(name,stype,dtype,rshift,gshift,bshift,rmask,gmask,bmask)    \
static void name(int sx, int sy, int sw, int sh)                                        \
{                                                                                       \
   int y, x;                                                                            \
   for (y = sy; y < (sy + sh); y++) {                                                   \
      stype *s = (stype*) (_xwin.screen_line[y]) + sx;                                  \
      dtype *d = (dtype*) (_xwin.buffer_line[y]) + sx;                                  \
      for (x = sw - 1; x >= 0; x--) {                                                   \
         unsigned long color = *s++;                                                    \
         *d++ = (_xwin.rmap[(color >> (rshift)) & (rmask)]                              \
                 | _xwin.gmap[(color >> (gshift)) & (gmask)]                            \
                 | _xwin.bmap[(color >> (bshift)) & (bmask)]);                          \
      }                                                                                 \
   }                                                                                    \
}

#define MAKE_FAST_TRUECOLOR24(name,dtype)                                               \
static void name(int sx, int sy, int sw, int sh)                                        \
{                                                                                       \
   int x, y;                                                                            \
   for (y = sy; y < (sy + sh); y++) {                                                   \
      unsigned char *s = _xwin.screen_line[y] + 3 * sx;                                 \
      dtype *d = (dtype*) (_xwin.buffer_line[y]) + sx;                                  \
      for (x = sw - 1; x >= 0; s += 3, x--) {                                           \
         *d++ = (_xwin.rmap[s[DEFAULT_RGB_R_POS_24]]                                    \
                 | _xwin.gmap[s[DEFAULT_RGB_G_POS_24]]                                  \
                 | _xwin.bmap[s[DEFAULT_RGB_B_POS_24]]);                                \
      }                                                                                 \
   }                                                                                    \
}

#define MAKE_FAST_TRUECOLOR_TO24(name,stype,rshift,gshift,bshift,rmask,gmask,bmask)     \
static void name(int sx, int sy, int sw, int sh)                                        \
{                                                                                       \
   int x, y;                                                                            \
   for (y = sy; y < (sy + sh); y++) {                                                   \
      stype *s = (stype*) (_xwin.screen_line[y]) + sx;                                  \
      unsigned char *d = _xwin.buffer_line[y] + 3 * sx;                                 \
      for (x = sw - 1; x >= 0; d += 3, x--) {                                           \
         unsigned long color = *s++;                                                    \
         color = (_xwin.rmap[(color >> (rshift)) & (rmask)]                             \
                  | _xwin.gmap[(color >> (gshift)) & (gmask)]                           \
                  | _xwin.bmap[(color >> (bshift)) & (bmask)]);                         \
         WRITE3BYTES(d, color);                                                         \
      }                                                                                 \
   }                                                                                    \
}

#define MAKE_FAST_TRUECOLOR24_TO24(name)                                                \
static void name(int sx, int sy, int sw, int sh)                                        \
{                                                                                       \
   int x, y;                                                                            \
   for (y = sy; y < (sy + sh); y++) {                                                   \
      unsigned char *s = _xwin.screen_line[y] + 3 * sx;                                 \
      unsigned char *d = _xwin.buffer_line[y] + 3 * sx;                                 \
      for (x = sw - 1; x >= 0; s += 3, d += 3, x--) {                                   \
         unsigned long color = _xwin.rmap[s[DEFAULT_RGB_R_POS_24]]                      \
                               | _xwin.gmap[s[DEFAULT_RGB_G_POS_24]]                    \
                               | _xwin.bmap[s[DEFAULT_RGB_B_POS_24]];                   \
         WRITE3BYTES(d, color);                                                         \
      }                                                                                 \
   }                                                                                    \
}

MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_8_to_8,
                    unsigned char, unsigned char, 0, 0, 0, 0xFF, 0xFF, 0xFF);
MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_8_to_16,
                    unsigned char, unsigned short, 0, 0, 0, 0xFF, 0xFF, 0xFF);
MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_8_to_32,
                    unsigned char, uint32_t, 0, 0, 0, 0xFF, 0xFF, 0xFF);

MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_15_to_8,
                    unsigned short, unsigned char, 0, 5, 10, 0x1F, 0x1F, 0x1F);
MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_15_to_16,
                    unsigned short, unsigned short, 0, 5, 10, 0x1F, 0x1F, 0x1F);
MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_15_to_32,
                    unsigned short, uint32_t, 0, 5, 10, 0x1F, 0x1F, 0x1F);

MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_16_to_8,
                    unsigned short, unsigned char, 0, 5, 11, 0x1F, 0x3F, 0x1F);
MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_16_to_16,
                    unsigned short, unsigned short, 0, 5, 11, 0x1F, 0x3F, 0x1F);
MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_16_to_32,
                    unsigned short, uint32_t, 0, 5, 11, 0x1F, 0x3F, 0x1F);

MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_32_to_8,
                    uint32_t, unsigned char, 0, 8, 16, 0xFF, 0xFF, 0xFF);
MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_32_to_16,
                    uint32_t, unsigned short, 0, 8, 16, 0xFF, 0xFF, 0xFF);
MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_32_to_32,
                    uint32_t, uint32_t, 0, 8, 16, 0xFF, 0xFF, 0xFF);

MAKE_FAST_TRUECOLOR24(_xwin_private_fast_truecolor_24_to_8,
                      unsigned char);
MAKE_FAST_TRUECOLOR24(_xwin_private_fast_truecolor_24_to_16,
                      unsigned short);
MAKE_FAST_TRUECOLOR24(_xwin_private_fast_truecolor_24_to_32,
                      uint32_t);

MAKE_FAST_TRUECOLOR_TO24(_xwin_private_fast_truecolor_8_to_24,
                         unsigned char, 0, 0, 0, 0xFF, 0xFF, 0xFF);
MAKE_FAST_TRUECOLOR_TO24(_xwin_private_fast_truecolor_15_to_24,
                         unsigned short, 0, 5, 10, 0x1F, 0x1F, 0x1F);
MAKE_FAST_TRUECOLOR_TO24(_xwin_private_fast_truecolor_16_to_24,
                         unsigned short, 0, 5, 11, 0x1F, 0x3F, 0x1F);
MAKE_FAST_TRUECOLOR_TO24(_xwin_private_fast_truecolor_32_to_24,
                         uint32_t, 0, 8, 16, 0xFF, 0xFF, 0xFF);

MAKE_FAST_TRUECOLOR24_TO24(_xwin_private_fast_truecolor_24_to_24);

#define MAKE_FAST_PALETTE8(name,dtype)                                                  \
static void name(int sx, int sy, int sw, int sh)                                        \
{                                                                                       \
   int x, y;                                                                            \
   for (y = sy; y < (sy + sh); y++) {                                                   \
      unsigned char *s = _xwin.screen_line[y] + sx;                                     \
      dtype *d = (dtype*) (_xwin.buffer_line[y]) + sx;                                  \
      for (x = sw - 1; x >= 0; x--) {                                                   \
         unsigned long color = *s++;                                                    \
         *d++ = _xwin.cmap[(_xwin.rmap[color]                                           \
                            | _xwin.gmap[color]                                         \
                            | _xwin.bmap[color])];                                      \
      }                                                                                 \
   }                                                                                    \
}

#define MAKE_FAST_PALETTE(name,stype,dtype,rshift,gshift,bshift)                        \
static void name(int sx, int sy, int sw, int sh)                                        \
{                                                                                       \
   int x, y;                                                                            \
   for (y = sy; y < (sy + sh); y++) {                                                   \
      stype *s = (stype*) (_xwin.screen_line[y]) + sx;                                  \
      dtype *d = (dtype*) (_xwin.buffer_line[y]) + sx;                                  \
      for (x = sw - 1; x >= 0; x--) {                                                   \
         unsigned long color = *s++;                                                    \
         *d++ = _xwin.cmap[((((color >> (rshift)) & 0x0F) << 8)                         \
                            | (((color >> (gshift)) & 0x0F) << 4)                       \
                            | ((color >> (bshift)) & 0x0F))];                           \
      }                                                                                 \
   }                                                                                    \
}

#define MAKE_FAST_PALETTE24(name,dtype)                                                 \
static void name(int sx, int sy, int sw, int sh)                                        \
{                                                                                       \
   int x, y;                                                                            \
   for (y = sy; y < (sy + sh); y++) {                                                   \
      unsigned char *s = _xwin.screen_line[y] + 3 * sx;                                 \
      dtype *d = (dtype*) (_xwin.buffer_line[y]) + sx;                                  \
      for (x = sw - 1; x >= 0; s += 3, x--) {                                           \
         *d++ = _xwin.cmap[((((unsigned long) s[DEFAULT_RGB_R_POS_24] << 4) & 0xF00)    \
                            | ((unsigned long) s[DEFAULT_RGB_G_POS_24] & 0xF0)          \
                            | (((unsigned long) s[DEFAULT_RGB_B_POS_24] >> 4) & 0x0F))];\
      }                                                                                 \
   }                                                                                    \
}

MAKE_FAST_PALETTE8(_xwin_private_fast_palette_8_to_8,
                   unsigned char);
MAKE_FAST_PALETTE8(_xwin_private_fast_palette_8_to_16,
                   unsigned short);
MAKE_FAST_PALETTE8(_xwin_private_fast_palette_8_to_32,
                   uint32_t);

MAKE_FAST_PALETTE(_xwin_private_fast_palette_15_to_8,
                  unsigned short, unsigned char, 1, 6, 11);
MAKE_FAST_PALETTE(_xwin_private_fast_palette_15_to_16,
                  unsigned short, unsigned short, 1, 6, 11);
MAKE_FAST_PALETTE(_xwin_private_fast_palette_15_to_32,
                  unsigned short, uint32_t, 1, 6, 11);

MAKE_FAST_PALETTE(_xwin_private_fast_palette_16_to_8,
                  unsigned short, unsigned char, 1, 7, 12);
MAKE_FAST_PALETTE(_xwin_private_fast_palette_16_to_16,
                  unsigned short, unsigned short, 1, 7, 12);
MAKE_FAST_PALETTE(_xwin_private_fast_palette_16_to_32,
                  unsigned short, uint32_t, 1, 7, 12);

MAKE_FAST_PALETTE(_xwin_private_fast_palette_32_to_8,
                  uint32_t, unsigned char, 4, 12, 20);
MAKE_FAST_PALETTE(_xwin_private_fast_palette_32_to_16,
                  uint32_t, unsigned short, 4, 12, 20);
MAKE_FAST_PALETTE(_xwin_private_fast_palette_32_to_32,
                  uint32_t, uint32_t, 4, 12, 20);

MAKE_FAST_PALETTE24(_xwin_private_fast_palette_24_to_8,
                    unsigned char);
MAKE_FAST_PALETTE24(_xwin_private_fast_palette_24_to_16,
                    unsigned short);
MAKE_FAST_PALETTE24(_xwin_private_fast_palette_24_to_32,
                    uint32_t);

#define MAKE_SLOW_TRUECOLOR(name,stype,rshift,gshift,bshift,rmask,gmask,bmask)          \
static void name(int sx, int sy, int sw, int sh)                                        \
{                                                                                       \
   int x, y;                                                                            \
   for (y = sy; y < (sy + sh); y++) {                                                   \
      stype *s = (stype*) (_xwin.screen_line[y]) + sx;                                  \
      for (x = sx; x < (sx + sw); x++) {                                                \
         unsigned long color = *s++;                                                    \
         XPutPixel (_xwin.ximage, x, y,                                                 \
                    (_xwin.rmap[(color >> (rshift)) & (rmask)]                          \
                     | _xwin.gmap[(color >> (gshift)) & (gmask)]                        \
                     | _xwin.bmap[(color >> (bshift)) & (bmask)]));                     \
      }                                                                                 \
   }                                                                                    \
}

#define MAKE_SLOW_TRUECOLOR24(name)                                                     \
static void name(int sx, int sy, int sw, int sh)                                        \
{                                                                                       \
   int x, y;                                                                            \
   for (y = sy; y < (sy + sh); y++) {                                                   \
      unsigned char *s = _xwin.screen_line[y] + 3 * sx;                                 \
      for (x = sx; x < (sx + sw); s += 3, x++) {                                        \
         XPutPixel(_xwin.ximage, x, y,                                                  \
                   (_xwin.rmap[s[DEFAULT_RGB_R_POS_24]]                                 \
                    | _xwin.gmap[s[DEFAULT_RGB_G_POS_24]]                               \
                    | _xwin.bmap[s[DEFAULT_RGB_B_POS_24]]));                            \
      }                                                                                 \
   }                                                                                    \
}

MAKE_SLOW_TRUECOLOR(_xwin_private_slow_truecolor_8, unsigned char, 0, 0, 0, 0xFF, 0xFF, 0xFF);
MAKE_SLOW_TRUECOLOR(_xwin_private_slow_truecolor_15, unsigned short, 0, 5, 10, 0x1F, 0x1F, 0x1F);
MAKE_SLOW_TRUECOLOR(_xwin_private_slow_truecolor_16, unsigned short, 0, 5, 11, 0x1F, 0x3F, 0x1F);
MAKE_SLOW_TRUECOLOR(_xwin_private_slow_truecolor_32, uint32_t, 0, 8, 16, 0xFF, 0xFF, 0xFF);
MAKE_SLOW_TRUECOLOR24(_xwin_private_slow_truecolor_24);

#define MAKE_SLOW_PALETTE8(name)                                                        \
static void name(int sx, int sy, int sw, int sh)                                        \
{                                                                                       \
   int x, y;                                                                            \
   for (y = sy; y < (sy + sh); y++) {                                                   \
      unsigned char *s = _xwin.screen_line[y] + sx;                                     \
      for (x = sx; x < (sx + sw); x++) {                                                \
         unsigned long color = *s++;                                                    \
         XPutPixel(_xwin.ximage, x, y,                                                  \
                   _xwin.cmap[(_xwin.rmap[color]                                        \
                               | _xwin.gmap[color]                                      \
                               | _xwin.bmap[color])]);                                  \
      }                                                                                 \
   }                                                                                    \
}

#define MAKE_SLOW_PALETTE(name,stype,rshift,gshift,bshift)                              \
static void name(int sx, int sy, int sw, int sh)                                        \
{                                                                                       \
   int x, y;                                                                            \
   for (y = sy; y < (sy + sh); y++) {                                                   \
      stype *s = (stype*) (_xwin.screen_line[y]) + sx;                                  \
      for (x = sx; x < (sx + sw); x++) {                                                \
         unsigned long color = *s++;                                                    \
         XPutPixel(_xwin.ximage, x, y,                                                  \
                   _xwin.cmap[((((color >> (rshift)) & 0x0F) << 8)                      \
                               | (((color >> (gshift)) & 0x0F) << 4)                    \
                               | ((color >> (bshift)) & 0x0F))]);                       \
      }                                                                                 \
   }                                                                                    \
}

#define MAKE_SLOW_PALETTE24(name)                                                       \
static void name(int sx, int sy, int sw, int sh)                                        \
{                                                                                       \
   int x, y;                                                                            \
   for (y = sy; y < (sy + sh); y++) {                                                   \
      unsigned char *s = _xwin.screen_line[y] + 3 * sx;                                 \
      for (x = sx; x < (sx + sw); s += 3, x++) {                                        \
         XPutPixel(_xwin.ximage, x, y,                                                  \
                   _xwin.cmap[((((unsigned long) s[DEFAULT_RGB_R_POS_24] << 4) & 0xF00) \
                               | ((unsigned long) s[DEFAULT_RGB_G_POS_24] & 0xF0)       \
                               | (((unsigned long) s[DEFAULT_RGB_B_POS_24] >> 4) & 0x0F))]); \
      }                                                                                 \
   }                                                                                    \
}

MAKE_SLOW_PALETTE8(_xwin_private_slow_palette_8);
MAKE_SLOW_PALETTE(_xwin_private_slow_palette_15, unsigned short, 1, 6, 11);
MAKE_SLOW_PALETTE(_xwin_private_slow_palette_16, unsigned short, 1, 7, 12);
MAKE_SLOW_PALETTE(_xwin_private_slow_palette_32, uint32_t, 4, 12, 20);
MAKE_SLOW_PALETTE24(_xwin_private_slow_palette_24);

/*
 * Functions for setting "hardware" colors in 8bpp modes.
 */
static void _xwin_private_set_matching_colors(AL_CONST PALETTE p, int from, int to)
{
   int i;
   static XColor color[256];

   for (i = from; i <= to; i++) {
      color[i].flags = DoRed | DoGreen | DoBlue;
      color[i].pixel = i;
      color[i].red = ((p[i].r & 0x3F) * 65535L) / 0x3F;
      color[i].green = ((p[i].g & 0x3F) * 65535L) / 0x3F;
      color[i].blue = ((p[i].b & 0x3F) * 65535L) / 0x3F;
   }
   XStoreColors(_xwin.display, _xwin.colormap, color + from, to - from + 1);
}

static void _xwin_private_set_truecolor_colors(AL_CONST PALETTE p, int from, int to)
{
   int i, rmax, gmax, bmax;

   rmax = _xwin.rsize - 1;
   gmax = _xwin.gsize - 1;
   bmax = _xwin.bsize - 1;
   for (i = from; i <= to; i++) {
      _xwin.rmap[i] = (((p[i].r & 0x3F) * rmax) / 0x3F) << _xwin.rshift;
      _xwin.gmap[i] = (((p[i].g & 0x3F) * gmax) / 0x3F) << _xwin.gshift;
      _xwin.bmap[i] = (((p[i].b & 0x3F) * bmax) / 0x3F) << _xwin.bshift;
   }
}

static void _xwin_private_set_palette_colors(AL_CONST PALETTE p, int from, int to)
{
   int i;

   for (i = from; i <= to; i++) {
      _xwin.rmap[i] = (((p[i].r & 0x3F) * 15) / 0x3F) << 8;
      _xwin.gmap[i] = (((p[i].g & 0x3F) * 15) / 0x3F) << 4;
      _xwin.bmap[i] = (((p[i].b & 0x3F) * 15) / 0x3F);
   }
}

static void _xwin_private_set_palette_range(AL_CONST PALETTE p, int from, int to)
{
   RGB *pal;
   int c;
   unsigned char temp;

   if (_xwin.set_colors != 0) {
      if (blitter_func) {
         if (use_bgr_palette_hack && (from >= 0) && (to < 256)) {
            pal = _AL_MALLOC_ATOMIC(sizeof(RGB)*256);
            ASSERT(pal);
            ASSERT(p);
            if (!pal || !p)
               return; /* ... in shame and disgrace */
            memcpy(&pal[from], &p[from], sizeof(RGB)*(to-from+1));
            for (c = from; c <= to; c++) {
               temp = pal[c].r;
               pal[c].r = pal[c].b;
               pal[c].b = temp;
            }
            _set_colorconv_palette(pal, from, to);
            _AL_FREE(pal);
         }
         else {
            _set_colorconv_palette(p, from, to);
         }
      }

      /* Set "hardware" colors.  */
      (*(_xwin.set_colors))(p, from, to);

      /* Update XImage and window.  */
      if (!_xwin.matching_formats)
         _xwin_private_update_screen(0, 0, _xwin.virtual_width, _xwin.virtual_height);
   }
}

void _xwin_set_palette_range(AL_CONST PALETTE p, int from, int to, int vsync)
{
   if (vsync) {
      _xwin_vsync();
   }

   XLOCK();
   _xwin_private_set_palette_range(p, from, to);
   XUNLOCK();
}



/* _xwin_set_window_defaults:
 *  Set default window parameters.
 */
static void _xwin_private_set_window_defaults(void)
{
   XClassHint hint;
   XWMHints wm_hints;

   if (_xwin.wm_window == None)
      return;

   /* Set window title.  */
   XStoreName(_xwin.display, _xwin.wm_window, _xwin.window_title);

   /* Set hints.  */
   hint.res_name = _xwin.application_name;
   hint.res_class = _xwin.application_class;
   XSetClassHint(_xwin.display, _xwin.wm_window, &hint);

   wm_hints.flags = InputHint | StateHint | WindowGroupHint;
   wm_hints.input = True;
   wm_hints.initial_state = NormalState;
   wm_hints.window_group = _xwin.wm_window;

#ifdef ALLEGRO_XWINDOWS_WITH_XPM
   if (allegro_icon) {
      wm_hints.flags |= IconPixmapHint | IconMaskHint;
      XpmCreatePixmapFromData(_xwin.display, _xwin.wm_window, allegro_icon,
         &wm_hints.icon_pixmap, &wm_hints.icon_mask, NULL);
   }
#endif

   XSetWMHints(_xwin.display, _xwin.wm_window, &wm_hints);
}



/* _xwin_flush_buffers:
 *  Flush input and output X-buffers.
 */
static void _xwin_private_flush_buffers(void)
{
   if (_xwin.display != 0)
      XSync(_xwin.display, False);
}

void _xwin_flush_buffers(void)
{
   XLOCK();
   _xwin_private_flush_buffers();
   XUNLOCK();
}



/* _xwin_vsync:
 *  Emulation of vsync.
 */
void _xwin_vsync(void)
{
   if (_timer_installed) {
      int prev = retrace_count;

      XLOCK();
      XSync(_xwin.display, False);
      XUNLOCK();

      do {
         rest(1);
      } while (retrace_count == prev);
   }
   else {
      /* This does not wait for the VBI - but it waits until X11 has
       * synchronized, i.e. until actual changes are visible. So it
       * has a similar effect.
       */
      XLOCK();
      XSync(_xwin.display, False);
      XUNLOCK();
   }
}


/* _xwin_process_event:
 *  Process one event.
 */
static void _xwin_private_process_event(XEvent *event)
{
   int dx, dy, dz = 0, dw = 0;
   static int mouse_buttons = 0;
   static int mouse_savedx = 0;
   static int mouse_savedy = 0;
   static int mouse_warp_now = 0;
   static int mouse_was_warped = 0;

   switch (event->type) {
      case KeyPress:
         _xwin_keyboard_handler(&event->xkey, FALSE);
         break;
      case KeyRelease:
         _xwin_keyboard_handler(&event->xkey, FALSE);
         break;
      case FocusIn:
         _switch_in();
         _xwin_keyboard_focus_handler(&event->xfocus);
         break;
      case FocusOut:
         _switch_out();
         _xwin_keyboard_focus_handler(&event->xfocus);
         break;
      case ButtonPress:
         /* Mouse button pressed.  */
         if (event->xbutton.button == Button1)
            mouse_buttons |= 1;
         else if (event->xbutton.button == Button3)
            mouse_buttons |= 2;
         else if (event->xbutton.button == Button2)
            mouse_buttons |= 4;
         else if (event->xbutton.button == Button4) {
            dz = 1;
         }
         else if (event->xbutton.button == Button5) {
            dz = -1;
         }
         else if (event->xbutton.button == 6) {
            dw = -1;
         }
         else if (event->xbutton.button == 7) {
            dw = 1;
         }
         else if (event->xbutton.button == 8)
            mouse_buttons |= 8;
         else if (event->xbutton.button == 9)
            mouse_buttons |= 16;
         if (_xwin_mouse_interrupt)
            (*_xwin_mouse_interrupt)(0, 0, dz, dw, mouse_buttons);
         break;
      case ButtonRelease:
         /* Mouse button released.  */
         if (event->xbutton.button == Button1)
            mouse_buttons &= ~1;
         else if (event->xbutton.button == Button3)
            mouse_buttons &= ~2;
         else if (event->xbutton.button == Button2)
            mouse_buttons &= ~4;
         else if (event->xbutton.button == 8)
            mouse_buttons &= ~8;
         else if (event->xbutton.button == 9)
            mouse_buttons &= ~16;
         if (_xwin_mouse_interrupt)
            (*_xwin_mouse_interrupt)(0, 0, 0, 0, mouse_buttons);
         break;
      case MotionNotify:
         /* Mouse moved.  */
         dx = event->xmotion.x - mouse_savedx;
         dy = event->xmotion.y - mouse_savedy;
         /* Discard some events after warp.  */
         if (mouse_was_warped && ((dx != 0) || (dy != 0)) && (mouse_was_warped++ < 16))
            break;
         mouse_savedx = event->xmotion.x;
         mouse_savedy = event->xmotion.y;
         mouse_was_warped = 0;
         if (!_xwin.mouse_warped) {
            /* Move Allegro cursor to X-cursor.  */
            dx = event->xmotion.x - (_mouse_x - (_xwin_mouse_extended_range ? _xwin.scroll_x : 0));
            dy = event->xmotion.y - (_mouse_y - (_xwin_mouse_extended_range ? _xwin.scroll_y : 0));
         }
         if (((dx != 0) || (dy != 0)) && _xwin_mouse_interrupt) {
            if (_xwin.mouse_warped && (mouse_warp_now++ & 4)) {
               /* Warp X-cursor to the center of the window.  */
               mouse_savedx = _xwin.window_width / 2;
               mouse_savedy = _xwin.window_height / 2;
               mouse_was_warped = 1;
               XWarpPointer(_xwin.display, _xwin.window, _xwin.window,
                            0, 0, _xwin.window_width, _xwin.window_height,
                            mouse_savedx, mouse_savedy);
            }
            /* Move Allegro cursor.  */
            (*_xwin_mouse_interrupt)(dx, dy, 0, 0, mouse_buttons);
         }
         break;
      case EnterNotify:
        /* Do not generate Enter/Leave notifications when
           XGrabPointer/XUngrabPointer() are called
           (NotifyGrab/NotifyUngrab modes). */
        if (event->xcrossing.mode != NotifyNormal)
           break;
         /* Mouse entered window.  */
         _mouse_on = TRUE;
         mouse_savedx = event->xcrossing.x;
         mouse_savedy = event->xcrossing.y;
         mouse_was_warped = 0;
         if (!_xwin.mouse_warped) {
            /* Move Allegro cursor to X-cursor.  */
            dx = event->xcrossing.x - (_mouse_x - (_xwin_mouse_extended_range ? _xwin.scroll_x : 0));
            dy = event->xcrossing.y - (_mouse_y - (_xwin_mouse_extended_range ? _xwin.scroll_y : 0));
            if (((dx != 0) || (dy != 0)) && _xwin_mouse_interrupt)
               (*_xwin_mouse_interrupt)(dx, dy, 0, 0, mouse_buttons);
         }
         else if (_xwin_mouse_interrupt)
            (*_xwin_mouse_interrupt)(0, 0, 0, 0, mouse_buttons);
         break;
      case LeaveNotify:
        if (event->xcrossing.mode != NotifyNormal)
           break;
         _mouse_on = FALSE;
         if (_xwin_mouse_interrupt)
            (*_xwin_mouse_interrupt)(0, 0, 0, 0, mouse_buttons);
         break;
      case Expose:
         /* Request to redraw part of the window.  */
         (*_xwin_window_redrawer)(event->xexpose.x, event->xexpose.y,
                                     event->xexpose.width, event->xexpose.height);
         break;
      case MappingNotify:
         /* Keyboard mapping changed.  */
         if (event->xmapping.request == MappingKeyboard)
            _xwin_get_keyboard_mapping();
         break;
      case ClientMessage:
         /* Window close request */
         if ((Atom)event->xclient.data.l[0] == wm_delete_window) {
            if (_xwin.close_button_callback)
               _xwin.close_button_callback();
         }
         break;
      case ConfigureNotify: {
         int old_width = _xwin.window_width;
         int old_height = _xwin.window_height;
         int new_width = event->xconfigure.width;
         int new_height = event->xconfigure.height;

         if (_xwin.resize_callback &&
             ((old_width != new_width) ||
              (old_height != new_height))) {
            RESIZE_DISPLAY_EVENT ev;
            ev.old_w = old_width;
            ev.old_h = old_height;
            ev.new_w = new_width;
            ev.new_h = new_height;
            ev.is_maximized = 0;
            ev.is_restored = 0;

            _xwin.resize_callback(&ev);
         }
         break;
      }
   }
}



/* _xwin_handle_input:
 *  Handle events from the queue.
 */
void _xwin_private_handle_input(void)
{
   int i, events, events_queued;
   static XEvent event[X_MAX_EVENTS + 1]; /* +1 for possible extra event, see below. */

   if (_xwin.display == 0)
      return;

   /* Switch mouse to non-warped mode if mickeys were not used recently (~2 seconds).  */
   if (_xwin.mouse_warped && (_xwin.mouse_warped++ > MOUSE_WARP_DELAY)) {
      _xwin.mouse_warped = 0;
      /* Move X-cursor to Allegro cursor.  */
      XWarpPointer(_xwin.display, _xwin.window, _xwin.window,
                   0, 0, _xwin.window_width, _xwin.window_height,
                   _mouse_x - (_xwin_mouse_extended_range ? _xwin.scroll_x : 0),
                   _mouse_y - (_xwin_mouse_extended_range ? _xwin.scroll_y : 0));
   }

   /* Flush X-buffers.  */
   _xwin_private_flush_buffers();

   /* How much events are available in the queue.  */
   events = events_queued = XEventsQueued(_xwin.display, QueuedAlready);
   if (events <= 0)
      return;

   /* Limit amount of events we read at once.  */
   if (events > X_MAX_EVENTS)
      events = X_MAX_EVENTS;

   /* Read pending events.  */
   for (i = 0; i < events; i++)
      XNextEvent(_xwin.display, &event[i]);

   /* Can't have a KeyRelease as last event, since it might be only half
    * of a key repeat pair. Also see comment below.
    */
   if (events_queued > events && event[i - 1].type == KeyRelease) {
      XNextEvent(_xwin.display, &event[i]);
      events++;
   }

   /* Process all events.  */
   for (i = 0; i < events; i++) {

      /* Hack to make Allegro's key[] array work despite of key repeat.
       * If a KeyRelease is sent at the same time as a KeyPress following
       * it with the same keycode, we ignore the release event.
       */
      if (event[i].type == KeyRelease && (i + 1) < events) {
         if (event[i + 1].type == KeyPress) {
            if (event[i].xkey.keycode == event[i + 1].xkey.keycode &&
               event[i].xkey.time == event[i + 1].xkey.time)
               continue;
         }
      }

      _xwin_private_process_event(&event[i]);
   }
}

void _xwin_handle_input(void)
{
#ifndef ALLEGRO_MULTITHREADED
   if (_xwin.lock_count) {
      ++_xwin_missed_input;
      return;
   }
#endif

   XLOCK();

   if (_xwin_input_handler)
      _xwin_input_handler();
   else
      _xwin_private_handle_input();

   XUNLOCK();
}



/* _xwin_set_warped_mouse_mode:
 *  Switch mouse handling into warped mode.
 */
static void _xwin_private_set_warped_mouse_mode(int permanent)
{
   /* Don't enable warp mode if the hardware cursor is being displayed */
   if (!_xwin.hw_cursor_ok)
      _xwin.mouse_warped = ((permanent) ? 1 : (MOUSE_WARP_DELAY*7/8));
}

void _xwin_set_warped_mouse_mode(int permanent)
{
   XLOCK();
   _xwin_private_set_warped_mouse_mode(permanent);
   XUNLOCK();
}



/* _xwin_redraw_window:
 *  Redraws part of the window.
 */
static void _xwin_private_redraw_window(int x, int y, int w, int h)
{
   if (_xwin.window == None)
      return;

   /* Clip updated region.  */
   if (x >= _xwin.screen_width)
      return;
   if (x < 0) {
      w += x;
      x = 0;
   }
   if (w >= (_xwin.screen_width - x))
      w = _xwin.screen_width - x;
   if (w <= 0)
      return;

   if (y >= _xwin.screen_height)
      return;
   if (y < 0) {
      h += y;
      y = 0;
   }
   if (h >= (_xwin.screen_height - y))
      h = _xwin.screen_height - y;
   if (h <= 0)
      return;

   if (!_xwin.ximage)
      XFillRectangle(_xwin.display, _xwin.window, _xwin.gc, x, y, w, h);
   else {
#ifdef ALLEGRO_XWINDOWS_WITH_SHM
      if (_xwin.use_shm)
         XShmPutImage(_xwin.display, _xwin.window, _xwin.gc, _xwin.ximage,
                      x + _xwin.scroll_x, y + _xwin.scroll_y, x, y, w, h, False);
      else
#endif
         XPutImage(_xwin.display, _xwin.window, _xwin.gc, _xwin.ximage,
                   x + _xwin.scroll_x, y + _xwin.scroll_y, x, y, w, h);
   }
}

void _xwin_redraw_window(int x, int y, int w, int h)
{
   _xwin_lock(NULL);
   (*_xwin_window_redrawer)(x, y, w, h);
   _xwin_unlock(NULL);
}



/* _xwin_scroll_screen:
 *  Scroll visible screen in window.
 */
static int _xwin_private_scroll_screen(int x, int y)
{
   _xwin.scroll_x = x;
   _xwin.scroll_y = y;
   (*_xwin_window_redrawer)(0, 0, _xwin.screen_width, _xwin.screen_height);
   _xwin_private_flush_buffers();

   return 0;
}

int _xwin_scroll_screen(int x, int y)
{
   int result;

   if (x < 0)
      x = 0;
   else if (x >= (_xwin.virtual_width - _xwin.screen_width))
      x = _xwin.virtual_width - _xwin.screen_width;
   if (y < 0)
      y = 0;
   else if (y >= (_xwin.virtual_height - _xwin.screen_height))
      y = _xwin.virtual_height - _xwin.screen_height;
   if ((_xwin.scroll_x == x) && (_xwin.scroll_y == y))
      return 0;

   _xwin_lock(NULL);
   result = _xwin_private_scroll_screen(x, y);
   _xwin_unlock(NULL);
   return result;
}



/* _xwin_update_screen:
 *  Update part of the screen.
 */
static void _xwin_private_update_screen(int x, int y, int w, int h)
{
   /* Update frame buffer with screen contents.  */
   if (_xwin.screen_to_buffer != 0) {
      /* Clip updated region.  */
      if (x >= _xwin.virtual_width)
         return;
      if (x < 0) {
         w += x;
         x = 0;
      }
      if (w >= (_xwin.virtual_width - x))
         w = _xwin.virtual_width - x;
      if (w <= 0)
         return;

      if (y >= _xwin.virtual_height)
         return;
      if (y < 0) {
         h += y;
         y = 0;
      }
      if (h >= (_xwin.virtual_height - y))
         h = _xwin.virtual_height - y;
      if (h <= 0)
         return;

      (*(_xwin.screen_to_buffer))(x, y, w, h);
   }

   /* Update window.  */
   (*_xwin_window_redrawer)(x - _xwin.scroll_x, y - _xwin.scroll_y, w, h);
}

void _xwin_update_screen(int x, int y, int w, int h)
{
   _xwin_lock(NULL);
   _xwin_private_update_screen(x, y, w, h);
   _xwin_unlock(NULL);
}



/* _xwin_set_window_title:
 *  Wrapper for XStoreName.
 */
static void _xwin_private_set_window_title(AL_CONST char *name)
{
   if (!name)
      _al_sane_strncpy(_xwin.window_title, XWIN_DEFAULT_WINDOW_TITLE, sizeof(_xwin.window_title));
   else
      _al_sane_strncpy(_xwin.window_title, name, sizeof(_xwin.window_title));

   if (_xwin.wm_window != None)
      XStoreName(_xwin.display, _xwin.wm_window, _xwin.window_title);
}

void _xwin_set_window_title(AL_CONST char *name)
{
   XLOCK();
   _xwin_private_set_window_title(name);
   XUNLOCK();
}



/* _xwin_sysdrv_set_window_name:
 *  Sets window name and group.
 */
static void _xwin_private_set_window_name(AL_CONST char *name, AL_CONST char *group)
{
   XClassHint hint;

   if (!name)
      _al_sane_strncpy(_xwin.application_name, XWIN_DEFAULT_APPLICATION_NAME, sizeof(_xwin.application_name));
   else
      _al_sane_strncpy(_xwin.application_name, name, sizeof(_xwin.application_name));

   if (!group)
      _al_sane_strncpy(_xwin.application_class, XWIN_DEFAULT_APPLICATION_CLASS, sizeof(_xwin.application_class));
   else
      _al_sane_strncpy(_xwin.application_class, group, sizeof(_xwin.application_class));

   if (_xwin.window != None) {
      hint.res_name = _xwin.application_name;
      hint.res_class = _xwin.application_class;
      XSetClassHint(_xwin.display, _xwin.window, &hint);
   }
}

void xwin_set_window_name(AL_CONST char *name, AL_CONST char *group)
{
   char tmp1[128], tmp2[128];

   do_uconvert(name, U_CURRENT, tmp1, U_ASCII, sizeof(tmp1));
   do_uconvert(group, U_CURRENT, tmp2, U_ASCII, sizeof(tmp2));

   XLOCK();
   _xwin_private_set_window_name(tmp1, tmp2);
   XUNLOCK();
}



/* _xwin_get_pointer_mapping:
 *  Wrapper for XGetPointerMapping.
 */
static int _xwin_private_get_pointer_mapping(unsigned char map[], int nmap)
{
   return ((_xwin.display == 0) ? -1 : XGetPointerMapping(_xwin.display, map, nmap));
}

int _xwin_get_pointer_mapping(unsigned char map[], int nmap)
{
   int num;
   XLOCK();
   num = _xwin_private_get_pointer_mapping(map, nmap);
   XUNLOCK();
   return num;
}



/* _xwin_write_line:
 *  Update last selected line and select new line.
 */
uintptr_t _xwin_write_line(BITMAP *bmp, int line)
{
   int new_line = line + bmp->y_ofs;
   if ((new_line != _xwin_last_line) && (!_xwin_in_gfx_call) && (_xwin_last_line >= 0))
      _xwin_update_screen(0, _xwin_last_line, _xwin.virtual_width, 1);
   _xwin_last_line = new_line;
   return (uintptr_t) (bmp->line[line]);
}



/* _xwin_unwrite_line:
 *  Update last selected line.
 */
void _xwin_unwrite_line(BITMAP *bmp)
{
   if ((!_xwin_in_gfx_call) && (_xwin_last_line >= 0))
      _xwin_update_screen(0, _xwin_last_line, _xwin.virtual_width, 1);
   _xwin_last_line = -1;
}



/*
 * Support for XF86VidMode extension.
 */
#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
/* qsort comparison function for sorting the modes */
static int cmpmodes(const void *va, const void *vb)
{
    const XF86VidModeModeInfo *a = *(const XF86VidModeModeInfo **)va;
    const XF86VidModeModeInfo *b = *(const XF86VidModeModeInfo **)vb;

    if (a->hdisplay == b->hdisplay)
        return b->vdisplay - a->vdisplay;
    else
        return b->hdisplay - a->hdisplay;
}

/* _xvidmode_private_set_fullscreen:
 *  Attempt to switch to a better matching video mode.
 *  Matching code for non exact match (smallest bigger res) rather shamelessly
 *  taken from SDL.
 */
static void _xvidmode_private_set_fullscreen(int w, int h, int *vidmode_width,
   int *vidmode_height)
{
   int vid_event_base, vid_error_base;
   int vid_major_version, vid_minor_version;
   int i;

   /* Test that display is local.  */
   if (!_xwin_private_display_is_local())
      return;

   /* Test for presence of VidMode extension.  */
   if (!XF86VidModeQueryExtension(_xwin.display, &vid_event_base, &vid_error_base)
       || !XF86VidModeQueryVersion(_xwin.display, &vid_major_version, &vid_minor_version))
      return;

   /* Get list of modelines.  */
   if (!XF86VidModeGetAllModeLines(_xwin.display, _xwin.screen,
                                   &_xwin.num_modes, &_xwin.modesinfo))
      return;

   /* Remember the mode to restore.  */
   _xwin.orig_modeinfo = _xwin.modesinfo[0];

   /* Search for an exact matching video mode.  */
   for (i = 0; i < _xwin.num_modes; i++) {
      if ((_xwin.modesinfo[i]->hdisplay == w) &&
          (_xwin.modesinfo[i]->vdisplay == h))
         break;
   }

   /* Search for a non exact match (smallest bigger res).  */
   if (i == _xwin.num_modes) {
      int best_width = 0, best_height = 0;
      qsort(_xwin.modesinfo, _xwin.num_modes, sizeof(void *), cmpmodes);
      for (i = _xwin.num_modes-1; i > 0; i--) {
          if (!best_width) {
              if ((_xwin.modesinfo[i]->hdisplay >= w) &&
                  (_xwin.modesinfo[i]->vdisplay >= h)) {
                  best_width = _xwin.modesinfo[i]->hdisplay;
                  best_height = _xwin.modesinfo[i]->vdisplay;
              }
          }
          else {
              if ((_xwin.modesinfo[i]->hdisplay != best_width) ||
                  (_xwin.modesinfo[i]->vdisplay != best_height)) {
                  i++;
                  break;
              }
          }
      }
   }

   /* Switch video mode.  */
   if ((_xwin.modesinfo[i] == _xwin.orig_modeinfo) ||
       !XF86VidModeSwitchToMode(_xwin.display, _xwin.screen,
         _xwin.modesinfo[i])) {
      *vidmode_width  = _xwin.orig_modeinfo->hdisplay;
      *vidmode_height = _xwin.orig_modeinfo->vdisplay;
      _xwin.orig_modeinfo = NULL;
   }
   else {
      *vidmode_width  = _xwin.modesinfo[i]->hdisplay;
      *vidmode_height = _xwin.modesinfo[i]->vdisplay;
      /* Only kept / set for compatibility with apps which check this.  */
      _xwin.mode_switched = 1;
   }

   /* Lock mode switching.  */
   XF86VidModeLockModeSwitch(_xwin.display, _xwin.screen, True);

   /* Set viewport. */
   XF86VidModeSetViewPort(_xwin.display, _xwin.screen, 0, 0);
}



/* free_modelines:
 *  Free mode lines.
 */
static void free_modelines(XF86VidModeModeInfo **modesinfo, int num_modes)
{
   int i;

   for (i = 0; i < num_modes; i++)
      if (modesinfo[i]->privsize > 0)
         XFree(modesinfo[i]->private);
   XFree(modesinfo);
}



/* _xvidmode_private_unset_fullscreen:
 *  Restore original video mode and window attributes.
 */
static void _xvidmode_private_unset_fullscreen(void)
{
   if (_xwin.num_modes > 0) {
      /* Unlock mode switching.  */
      XF86VidModeLockModeSwitch(_xwin.display, _xwin.screen, False);

      if (_xwin.orig_modeinfo) {
         /* Restore the original video mode.  */
         XF86VidModeSwitchToMode(_xwin.display, _xwin.screen,
            _xwin.orig_modeinfo);
         _xwin.orig_modeinfo = 0;
         /* only kept / set for compatibility with apps which check this */
         _xwin.mode_switched = 0;
      }

      /* Free modelines.  */
      free_modelines(_xwin.modesinfo, _xwin.num_modes);
      _xwin.num_modes = 0;
      _xwin.modesinfo = 0;
   }
}
#endif



/* _xwin_private_fetch_mode_list:
 *  Generates a list of valid video modes.
 */
static GFX_MODE_LIST *_xwin_private_fetch_mode_list(void)
{
   int num_modes = 1, num_bpp = 0;
   GFX_MODE_LIST *mode_list;
   int i, j;
#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
   int has_vidmode = 0;
   int vid_event_base, vid_error_base;
   int vid_major_version, vid_minor_version;
   XF86VidModeModeInfo **modesinfo;

   /* Test that display is local.  */
   if (_xwin_private_display_is_local() &&
       /* Test for presence of VidMode extension.  */
       XF86VidModeQueryExtension(_xwin.display, &vid_event_base, &vid_error_base) &&
       XF86VidModeQueryVersion(_xwin.display, &vid_major_version, &vid_minor_version) &&
       /* Get list of modelines.  */
       XF86VidModeGetAllModeLines(_xwin.display, _xwin.screen, &num_modes, &modesinfo))
      has_vidmode = 1;
#endif

   /* Calculate the number of color depths we have to support.  */
#ifdef ALLEGRO_COLOR8
   num_bpp++;
#endif
#ifdef ALLEGRO_COLOR16
   num_bpp += 2;     /* 15, 16 */
#endif
#ifdef ALLEGRO_COLOR24
   num_bpp++;
#endif
#ifdef ALLEGRO_COLOR32
   num_bpp++;
#endif
   if (num_bpp == 0) /* ha! */
      return 0;

   /* Allocate space for mode list.  */
   mode_list = _AL_MALLOC(sizeof(GFX_MODE_LIST));
   if (!mode_list) {
#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
      if (has_vidmode)
         free_modelines(modesinfo, num_modes);
#endif
      return 0;
   }

   mode_list->mode = _AL_MALLOC(sizeof(GFX_MODE) * ((num_modes * num_bpp) + 1));
   if (!mode_list->mode) {
      _AL_FREE(mode_list);
#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
      if (has_vidmode)
         free_modelines(modesinfo, num_modes);
#endif
      return 0;
   }

   /* Fill in mode list.  */
   j = 0;
   for (i = 0; i < num_modes; i++) {

#define ADD_SCREEN_MODE(BPP)                                                  \
      mode_list->mode[j].width  = DisplayWidth(_xwin.display, _xwin.screen);  \
      mode_list->mode[j].height = DisplayHeight(_xwin.display, _xwin.screen); \
      mode_list->mode[j].bpp = BPP;                                           \
      j++
#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
#define ADD_VIDMODE_MODE(BPP)                               \
      mode_list->mode[j].width = modesinfo[i]->hdisplay;    \
      mode_list->mode[j].height = modesinfo[i]->vdisplay;   \
      mode_list->mode[j].bpp = BPP;                         \
      j++
#define ADD_MODE(BPP)                                       \
      if (has_vidmode) {                                    \
         ADD_VIDMODE_MODE(BPP);                             \
      } else {                                              \
         ADD_SCREEN_MODE(BPP);                              \
      }
#else
#define ADD_MODE(BPP) ADD_SCREEN_MODE(BPP)
#endif

#ifdef ALLEGRO_COLOR8
      ADD_MODE(8);
#endif
#ifdef ALLEGRO_COLOR16
      ADD_MODE(15);
      ADD_MODE(16);
#endif
#ifdef ALLEGRO_COLOR24
      ADD_MODE(24);
#endif
#ifdef ALLEGRO_COLOR32
      ADD_MODE(32);
#endif
   }

   mode_list->mode[j].width = 0;
   mode_list->mode[j].height = 0;
   mode_list->mode[j].bpp = 0;
   mode_list->num_modes = j;

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
   if (has_vidmode)
      free_modelines(modesinfo, num_modes);
#endif

   return mode_list;
}



/* _xwin_fetch_mode_list:
 *  Fetch mode list.
 */
GFX_MODE_LIST *_xwin_fetch_mode_list(void)
{
   GFX_MODE_LIST *list;
   XLOCK();
   list = _xwin_private_fetch_mode_list();
   XUNLOCK();
   return list;
}



/* Hook functions */
int (*_xwin_window_creator)(void) = &_xwin_private_create_window;
void (*_xwin_window_defaultor)(void) = &_xwin_private_set_window_defaults;
void (*_xwin_window_redrawer)(int, int, int, int) = &_xwin_private_redraw_window;
void (*_xwin_input_handler)(void) = 0;

void (*_xwin_keyboard_callback)(int, int) = 0;
