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
 *      Video driver using SVGAlib.
 *
 *      By Stefan T. Boettner.
 * 
 *      Modified extensively by Peter Wang.
 *
 *      Horizontal scrolling fixed by Attila Szilagyi.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"
#include "linalleg.h"


#if (defined ALLEGRO_LINUX_SVGALIB) && ((!defined ALLEGRO_WITH_MODULES) || (defined ALLEGRO_MODULE))

#include <signal.h>
#include <termios.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <sys/ioctl.h>
#include <vga.h>



static BITMAP *svga_init(int w, int h, int v_w, int v_h, int color_depth);
static void svga_exit(BITMAP *b);
static int  svga_scroll(int x, int y);
static void svga_vsync(void);
static void svga_set_palette(AL_CONST RGB *p, int from, int to, int vsync);
static void svga_save(void);
static void svga_restore(void);
/* static GFX_MODE_LIST *svga_fetch_mode_list(void); */

#ifndef ALLEGRO_NO_ASM
unsigned long _svgalib_read_line_asm(BITMAP *bmp, int line);
unsigned long _svgalib_write_line_asm(BITMAP *bmp, int line);
void _svgalib_unwrite_line_asm(BITMAP *bmp);
#endif



GFX_DRIVER gfx_svgalib = 
{
   GFX_SVGALIB,
   empty_string,
   empty_string,
   "SVGAlib", 
   svga_init,
   svga_exit,
   svga_scroll,
   svga_vsync,
   svga_set_palette,
   NULL, NULL, NULL,             /* no triple buffering */
   NULL, NULL, NULL, NULL,       /* no video bitmaps */
   NULL, NULL,                   /* no system bitmaps */
   NULL, NULL, NULL, NULL,       /* no hardware cursor */
   NULL,                         /* no drawing mode hook */
   svga_save,
   svga_restore,
   NULL,                        /* AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a)); */
   NULL,			 /* svga_fetch_mode_list disabled */
   0, 0,
   TRUE,
   0, 0, 0, 0, FALSE
};



static char svga_desc[256] = EMPTY_STRING;

static int svga_mode;

static unsigned int bytes_per_line;
static unsigned int display_start_mask;

static unsigned char *screen_buffer;
static int last_line;

static int saved_palette[PAL_SIZE * 3];



#ifdef ALLEGRO_MODULE

int _module_has_registered_via_atexit = 0;

#endif



/* _svgalib_read_line:
 *  Return linear offset for reading line.
 */
unsigned long _svgalib_read_line(BITMAP *bmp, int line)
{
   ASSERT(bmp);
   ASSERT(line>=0 && line<bmp->h);

   return (unsigned long) (bmp->line[line]);
}



/* _svgalib_write_line:
 *  Update last selected line and select new line.
 */
unsigned long _svgalib_write_line(BITMAP *bmp, int line)
{
   int new_line;
   ASSERT(bmp);
   ASSERT(line>=0 && line<bmp->h);

   new_line = line + bmp->y_ofs;
   if ((new_line != last_line) && (last_line >= 0))
      vga_drawscansegment(screen_buffer + last_line * bytes_per_line, 0, last_line, bytes_per_line);
   last_line = new_line;
   return (unsigned long) (bmp->line[line]);
}



/* _svgalib_unwrite_line:
 *  Update last selected line.
 */
void _svgalib_unwrite_line(BITMAP *bmp)
{
   ASSERT(bmp);

   if (last_line >= 0) {
      vga_drawscanline(last_line, screen_buffer + last_line * bytes_per_line);
      last_line = -1;
   }
}



/* save_signals, restore_signals:
 *  Helpers to save and restore signals captured by SVGAlib.
 */
static const int signals[] = {
   SIGUSR1, SIGUSR2,
   SIGHUP, SIGINT, SIGQUIT, SIGILL,
   SIGTRAP, SIGIOT, SIGBUS, SIGFPE,
   SIGSEGV, SIGPIPE, SIGALRM, SIGTERM,
   SIGXCPU, SIGXFSZ, SIGVTALRM, SIGPWR
};

#define NUM_SIGNALS	(sizeof (signals) / sizeof (int))

static struct sigaction old_signals[NUM_SIGNALS];

static void save_signals(void)
{
   int i;
   for (i = 0; i < (int)NUM_SIGNALS; i++) 
      sigaction(signals[i], NULL, old_signals+i);
}

static void restore_signals(void)
{
   int i;
   for (i = 0; i < (int)NUM_SIGNALS; i++)
      sigaction(signals[i], old_signals+i, NULL);
}



/* safe_vga_setmode:
 *  We don't want SVGAlib messing with our keyboard driver or taking 
 *  over control of VT switching.  Note that doing all this every 
 *  time is possibly a little excessive.  
 */
static int safe_vga_setmode(int num, int tio)
{
   struct termios termio;
   struct vt_mode vtm;
   int ret;

   save_signals();
   if (tio) 
      ioctl(__al_linux_console_fd, VT_GETMODE, &vtm);
   tcgetattr(__al_linux_console_fd, &termio);

   ret = vga_setmode(num);

#ifdef ALLEGRO_MODULE
   /* A side-effect of vga_setmode() is that it will register an
    * atexit handler.  See umodules.c for this problem.
    */
   _module_has_registered_via_atexit = 1;
#endif

   tcsetattr(__al_linux_console_fd, TCSANOW, &termio);
   if (tio) 
      ioctl(__al_linux_console_fd, VT_SETMODE, &vtm);
   restore_signals();

   return ret;
}



/* get_depth:
 *  SVGAlib speaks in number of colors and bytes per pixel.
 *  Allegro speaks in color depths.
 */
static int get_depth(int ncolors, int bytesperpixel)
{
   if (ncolors == 256) return 8;
   if (ncolors == 32768) return 15;
   if (ncolors == 65536) return 16;
   if (bytesperpixel == 3) return 24;
   if (bytesperpixel == 4) return 32;
   return -1;
}



/* mode_ok:
 *  Check if the mode passed matches the size and color depth requirements.
 */
static int mode_ok(vga_modeinfo *info, int w, int h, int v_w, int v_h, 
		   int color_depth)
{
   ASSERT(info);

   return ((color_depth == get_depth(info->colors, info->bytesperpixel))
	   && (((info->width == w) && (info->height == h))
	       || ((w == 0) && (h == 0)))
	   && (info->maxlogicalwidth >= (MAX(w, v_w) * info->bytesperpixel))
	   && (info->maxpixels >= (MAX(w, v_w) * MAX(h, v_h))));
}



/* find_and_set_mode:
 *  Helper to find a suitable video mode and then set it.
 */
static vga_modeinfo *find_and_set_mode(int w, int h, int v_w, int v_h,
				       int color_depth, int flags)
{
   vga_modeinfo *info;
   int i;
    
   for (i = 0; i <= vga_lastmodenumber(); i++) {
      if (!vga_hasmode(i)) 
	 continue;
      
      info = vga_getmodeinfo(i);
      if ((info->flags & IS_MODEX)
	  || ((flags) && !(info->flags & flags))
	  || (!mode_ok(info, w, h, v_w, v_h, color_depth)))
	 continue;
      
      if (safe_vga_setmode(i, 1) == 0) {
	 svga_mode = i;
	 gfx_svgalib.w = vga_getxdim();
	 gfx_svgalib.h = vga_getydim();
	 return info;
      }
   }

   return NULL;
}



/* set_color_shifts:
 *  Set the color shift values for truecolor modes.
 */
static void set_color_shifts(int color_depth, int bgr)
{
   switch (color_depth) {

      #ifdef ALLEGRO_COLOR16

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

      #endif

      #ifdef ALLEGRO_COLOR24

         case 24:
            if (bgr) {
               _rgb_r_shift_24 = 0;
               _rgb_g_shift_24 = 8;
               _rgb_b_shift_24 = 16;
	    }
            else {
               _rgb_r_shift_24 = 16;
               _rgb_g_shift_24 = 8;
               _rgb_b_shift_24 = 0;
	    }
            break;

      #endif

      #ifdef ALLEGRO_COLOR32

         case 32:
            if (bgr) {
               _rgb_a_shift_32 = 0;
               _rgb_r_shift_32 = 8;
               _rgb_g_shift_32 = 16;
               _rgb_b_shift_32 = 24;
	    }
            else {
               _rgb_a_shift_32 = 24;
               _rgb_r_shift_32 = 16;
               _rgb_g_shift_32 = 8;
               _rgb_b_shift_32 = 0;
	    }
            break;
       
      #endif
   }
}



/* do_set_mode:
 *  Do the hard work of setting a video mode, then return a screen bitmap.
 */
static BITMAP *do_set_mode(int w, int h, int v_w, int v_h, int color_depth)
{
   int vid_mem, width, height;
   vga_modeinfo *info;
   BITMAP *bmp;
   char tmp[128];

   /* Try get a linear frame buffer.  */

   info = find_and_set_mode(w, h, v_w, v_h, color_depth, CAPABLE_LINEAR);
   if (info) { 
      vid_mem = vga_setlinearaddressing();
      if (vid_mem < 0) {
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Cannot enable linear addressing"));
	 return NULL;
      }

      if ((v_w != 0) && (w != v_w)) {
	 if ((v_w < w) ||
	     ((v_w * info->bytesperpixel) %
	      ((info->flags & EXT_INFO_AVAILABLE) ? info->linewidth_unit : 8))) {
	    ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Invalid virtual resolution requested"));
	    return NULL;
	 }
	 bytes_per_line = v_w * info->bytesperpixel;
	 vga_setlogicalwidth(bytes_per_line);
      }
      else {
	 bytes_per_line = info->linewidth;
      }

      width = bytes_per_line / info->bytesperpixel;
      height = info->maxpixels / width;

      /* Set entries in gfx_svgalib.  */
      gfx_svgalib.vid_mem = vid_mem;
      gfx_svgalib.scroll = svga_scroll;

      ustrzcpy(svga_desc, sizeof(svga_desc), uconvert_ascii("SVGAlib (linear)", tmp));
      gfx_svgalib.desc = svga_desc;

      /* For hardware scrolling.  */
      display_start_mask = info->startaddressrange;

      /* Set truecolor format.  */
      set_color_shifts(color_depth, (info->flags & RGB_MISORDERED));

      /* Make the screen bitmap.  */
      return _make_bitmap(width, height,
			  (unsigned long)vga_getgraphmem(),
			  &gfx_svgalib, color_depth, bytes_per_line);
   }

   /* Try get a banked frame buffer.  */
   
   /* We don't support virtual screens larger than the screen itself 
    * in banked mode.  */
   if ((v_w > w) || (v_h > h)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
      return NULL;
   }

   info = find_and_set_mode(w, h, v_w, v_h, color_depth, 0);
   if (info) {
      width = gfx_svgalib.w;
      height = gfx_svgalib.h;
      bytes_per_line = width * info->bytesperpixel;
      vid_mem = bytes_per_line * height;

      /* Allocate memory buffer for screen.  */
      screen_buffer = _AL_MALLOC_ATOMIC(vid_mem);
      if (!screen_buffer) 
	 return NULL;
      last_line = -1;
       
      /* Set entries in gfx_svgalib.  */
      gfx_svgalib.vid_mem = vid_mem;
      gfx_svgalib.scroll = NULL;

      ustrzcpy(svga_desc, sizeof(svga_desc), uconvert_ascii("SVGAlib (banked)", tmp));
      gfx_svgalib.desc = svga_desc;

      /* Set truecolor format.  */
      set_color_shifts(color_depth, 0);

      /* Make the screen bitmap.  */
      bmp = _make_bitmap(width, height, (unsigned long)screen_buffer,
			 &gfx_svgalib, color_depth, bytes_per_line);
      if (bmp) {
	 /* Set bank switching routines.  */
#ifndef ALLEGRO_NO_ASM
	 bmp->read_bank = _svgalib_read_line_asm;
	 bmp->write_bank = _svgalib_write_line_asm;
	 bmp->vtable->unwrite_bank = _svgalib_unwrite_line_asm;
#else
	 bmp->read_bank = _svgalib_read_line;
	 bmp->write_bank = _svgalib_write_line;
	 bmp->vtable->unwrite_bank = _svgalib_unwrite_line;
#endif
      }

      return bmp;
   }

   ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
   return NULL;
}



/* svga_version2:
 *  Returns non-zero if we have SVGAlib version 2 (or the prereleases).
 */
static int svga_version2(void)
{
   #ifdef ALLEGRO_LINUX_SVGALIB_HAVE_VGA_VERSION
      return vga_version >= 0x1900;
   #else
      return 0;
   #endif
}



/* svga_init:
 *  Entry point to set a video mode.
 */
static BITMAP *svga_init(int w, int h, int v_w, int v_h, int color_depth)
{
   static int virgin = 1;
   
#ifndef ALLEGRO_HAVE_LIBPTHREAD
   /* SVGAlib and the SIGALRM code don't like each other, so only support
    * pthreads event processing.  */
   if (_unix_bg_man == &_bg_man_sigalrm)
      return NULL;
#endif

   if ((!svga_version2()) && (!__al_linux_have_ioperms)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("This driver needs root privileges"));
      return NULL;
   }

   __al_linux_use_console();

   /* Initialise SVGAlib.  */
   if (virgin) {
      if (!svga_version2())
	 seteuid(0);
      else {
	 /* I don't remember why I put this code in eight years ago (!) and the
	  * log message wasn't very good :-P  My guess is that a previous
	  * version of SVGAlib 1.9.x would call exit() if the /dev/svga device
	  * wasn't present.  However, as far as I know, the svgalib_helper
	  * doesn't compile with recent kernels so the device won't exist. --pw
	  */
#if 0
	 /* Avoid having SVGAlib calling exit() on us.  */
	 int fd = open("/dev/svga", O_RDWR);
	 if (fd < 0)
	    return NULL;
	 close(fd);
#endif
      }
      vga_disabledriverreport();
      if (vga_init() != 0)
	 return NULL;
      if (!svga_version2())
	 seteuid(getuid());
      virgin = 0;
   }
    
   /* Ask for a video mode.  */
   return do_set_mode(w, h, v_w, v_h, color_depth);
}



/* svga_exit:
 *  Unsets the video mode.
 */
static void svga_exit(BITMAP *b)
{
   if (screen_buffer) {
      _AL_FREE(screen_buffer);
      screen_buffer = NULL;
   }

   safe_vga_setmode(TEXT, 1);
   __al_linux_leave_console();
}



/* svga_scroll:
 *  Hardware scrolling routine.
 */
static int svga_scroll(int x, int y)
{
   vga_setdisplaystart((x + y * bytes_per_line) /* & display_start_mask */);
   /* The bitmask seems to mess things up on my machine, even though
    * the documentation says it should be there. -- PW  */

   /* wait for a retrace */
   if (_wait_for_vsync)
      vga_waitretrace();

   return 0;
}



/* svga_vsync:
 *  Waits for a retrace.
 */
static void svga_vsync(void)
{
   vga_waitretrace();
}



/* svga_set_palette:
 *  Sets the palette.
 */
static void svga_set_palette(AL_CONST RGB *p, int from, int to, int vsync)
{
   int i;
   ASSERT(p);
   ASSERT(from>=0 && from<=255);
   ASSERT(to>=0 && to<=255);
   ASSERT(from<=to);

   if (vsync)
      vga_waitretrace();

   for (i = from; i <= to; i++)
      vga_setpalette(i, p[i].r, p[i].g, p[i].b);
}



/* svga_save:
 *  Saves the graphics state.
 */
static void svga_save(void)
{
   vga_getpalvec(0, PAL_SIZE, saved_palette);
   safe_vga_setmode(TEXT, 0);
}



/* svga_restore:
 *  Restores the graphics state.
 */
static void svga_restore(void)
{
   safe_vga_setmode(svga_mode, 0);
   vga_setpage(0);
   vga_setpalvec(0, PAL_SIZE, saved_palette);
}



/* svga_fetch_mode_list:
 *  Generates a list of valid video modes.
 *  Returns the mode list on success or NULL on failure.
 *
 *  Disabled because it causes problems if called when other graphics
 *  drivers are being used.  (It starts up SVGAlib, which then does
 *  stuff with the console.)
 */
#if 0
static GFX_MODE_LIST *svga_fetch_mode_list(void)
{
   GFX_MODE_LIST *mode_list;
   vga_modeinfo *info;
   int i, count, bpp;

   if ((!svga_version2()) && (!__al_linux_have_ioperms))
      return NULL;

   for (i = 0, count = 0; i <= vga_lastmodenumber(); i++) {
      if (vga_hasmode(i)) {
	 info = vga_getmodeinfo(i);
	 if (!(info->flags & IS_MODEX))
	    count++;
      }
   }

   mode_list = _AL_MALLOC(sizeof(GFX_MODE_LIST));
   if (!mode_list)
      return NULL;

   mode_list->mode = _AL_MALLOC(sizeof(GFX_MODE) * (count + 1));
   if (!mode_list->mode) {
       _AL_FREE(mode_list);
       return NULL;
   }

   for (i = 0, count = 0; i <= vga_lastmodenumber(); i++) {
      if (!vga_hasmode(i))
	 continue;
      info = vga_getmodeinfo(i);
      if (info->flags & IS_MODEX)
	 continue;
      bpp = get_depth(info->colors, info->bytesperpixel);
      if (bpp < 0)
	 continue;
      mode_list->mode[count].width = info->width;
      mode_list->mode[count].height = info->height;
      mode_list->mode[count].bpp = bpp;
      count++;
   }
   
   mode_list->mode[count].width = 0;
   mode_list->mode[count].height = 0;
   mode_list->mode[count].bpp = 0;

   mode_list->num_modes = count;

   return mode_list;
}
#endif



#ifdef ALLEGRO_MODULE

/* _module_init:
 *  Called when loaded as a dynamically linked module.
 */
void _module_init(int system_driver)
{
   if (system_driver == SYSTEM_LINUX)
      _unix_register_gfx_driver(GFX_SVGALIB, &gfx_svgalib, TRUE, FALSE);
}

#endif      /* ifdef ALLEGRO_MODULE */



#endif      /* if (defined ALLEGRO_LINUX_SVGALIB) ... */
