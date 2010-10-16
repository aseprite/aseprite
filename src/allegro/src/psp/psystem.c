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
 *      PSP system driver.
 *
 *      By diedel.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintpsp.h"
//#include <malloc.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdebug.h>

#ifndef ALLEGRO_PSP
   #error something is wrong with the makefile
#endif


#define DEFAULT_SCREEN_WIDTH       480
#define DEFAULT_SCREEN_HEIGHT      272
#define DEFAULT_COLOR_DEPTH         16


static int psp_sys_init(void);
static void psp_sys_exit(void);
static void psp_message(AL_CONST char *msg);
static void psp_created_sub_bitmap(BITMAP *bmp, BITMAP *parent);
static void psp_get_gfx_safe_mode(int *driver, struct GFX_MODE *mode);



SYSTEM_DRIVER system_psp =
{
   SYSTEM_PSP,
   empty_string,
   empty_string,
   "PlayStation Portable",
   psp_sys_init,
   psp_sys_exit,
   NULL,  /* AL_METHOD(void, get_executable_name, (char *output, int size)); */
   NULL,  /* AL_METHOD(int, find_resource, (char *dest, AL_CONST char *resource, int size)); */
   NULL,  /* AL_METHOD(void, set_window_title, (AL_CONST char *name)); */
   NULL,  /* AL_METHOD(int, set_close_button_callback, (AL_METHOD(void, proc, (void)))); */
   psp_message,  /* AL_METHOD(void, message, (AL_CONST char *msg)); */
   NULL,  /* AL_METHOD(void, assert, (AL_CONST char *msg)); */
   NULL,  /* AL_METHOD(void, save_console_state, (void)); */
   NULL,  /* AL_METHOD(void, restore_console_state, (void)); */
   NULL,  /* AL_METHOD(struct BITMAP *, create_bitmap, (int color_depth, int width, int height)); */
   NULL,  /* AL_METHOD(void, created_bitmap, (struct BITMAP *bmp)); */
   NULL,  /* AL_METHOD(struct BITMAP *, create_sub_bitmap, (struct BITMAP *parent, int x, int y, int width, int height)); */
   psp_created_sub_bitmap,  /* AL_METHOD(void, created_sub_bitmap, (struct BITMAP *bmp, struct BITMAP *parent)); */
   NULL,  /* AL_METHOD(int, destroy_bitmap, (struct BITMAP *bitmap)); */
   NULL,  /* AL_METHOD(void, read_hardware_palette, (void)); */
   NULL,  /* AL_METHOD(void, set_palette_range, (AL_CONST struct RGB *p, int from, int to, int retracesync)); */
   NULL,  /* AL_METHOD(struct GFX_VTABLE *, get_vtable, (int color_depth)); */
   NULL,  /* AL_METHOD(int, set_display_switch_mode, (int mode)); */
   NULL,  /* AL_METHOD(void, display_switch_lock, (int lock, int foreground)); */
   NULL,  /* AL_METHOD(int, desktop_color_depth, (void)); */
   NULL,  /* AL_METHOD(int, get_desktop_resolution, (int *width, int *height)); */
   psp_get_gfx_safe_mode,  /*AL_METHOD(void, get_gfx_safe_mode, (int *driver, struct GFX_MODE *mode));*/
   NULL,  /* AL_METHOD(void, yield_timeslice, (void)); */
   NULL,  /* AL_METHOD(void *, create_mutex, (void)); */
   NULL,  /* AL_METHOD(void, destroy_mutex, (void *handle)); */
   NULL,  /* AL_METHOD(void, lock_mutex, (void *handle)); */
   NULL,  /* AL_METHOD(void, unlock_mutex, (void *handle)); */
   NULL,  /* AL_METHOD(_DRIVER_INFO *, gfx_drivers, (void)); */
   NULL,  /* AL_METHOD(_DRIVER_INFO *, digi_drivers, (void)); */
   NULL,  /* AL_METHOD(_DRIVER_INFO *, midi_drivers, (void)); */
   NULL,  /* AL_METHOD(_DRIVER_INFO *, keyboard_drivers, (void)); */
   NULL,  /* AL_METHOD(_DRIVER_INFO *, mouse_drivers, (void)); */
   NULL,  /* AL_METHOD(_DRIVER_INFO *, joystick_drivers, (void)); */
   NULL   /* AL_METHOD(_DRIVER_INFO *, timer_drivers, (void)); */
};



/* psp_sys_init:
 *  Initializes the PSP system driver.
 */
static int psp_sys_init(void)
{
   /* Setup OS type & version */
   os_type = OSTYPE_PSP;

   return 0;
}



/* psp_sys_exit:
 *  Shuts down the system driver.
 */
static void psp_sys_exit(void)
{
   sceKernelExitGame();
}



/* psp_message:
 *  Prints a text message in the PSP system dependent format.
 */
static void psp_message(AL_CONST char *msg)
{
   SceCtrlData pad;
   int buffers_to_read = 1;

   pspDebugScreenInit();
   pspDebugScreenPrintf(msg);

   /* The message is displayed until a button is pressed. */
   _psp_init_controller(SAMPLING_CYCLE, SAMPLING_MODE);
   do {
      sceCtrlPeekBufferPositive(&pad, buffers_to_read);
   } while (!pad.Buttons);
}



/* psp_create_bitmap:
 *  Creates a RAM bitmap with proper PSP pitch.
 */
BITMAP * psp_create_bitmap(int color_depth, int width, int height)
{
   GFX_VTABLE *vtable;
   BITMAP *bitmap;
   int nr_pointers;
   int i;
   int pitch;

   vtable = _get_vtable(color_depth);
   if (!vtable)
      return NULL;

   /* We need at least two pointers when drawing, otherwise we get crashes with
    * Electric Fence.  We think some of the assembly code assumes a second line
    * pointer is always available.
    */
   nr_pointers = MAX(2, height);
   bitmap = _AL_MALLOC(sizeof(BITMAP) + (sizeof(char *) * nr_pointers));
   if (!bitmap)
      return NULL;

   /* The memory bitmap width must be multiple of 8 pixels
    * in order to blit properly using sceGuCopyImage().
    */
   pitch = ALIGN_TO(width, 8);

   //bitmap->dat = memalign(64, pitch * height * BYTES_PER_PIXEL(color_depth));
   bitmap->dat = _AL_MALLOC_ATOMIC(pitch * height * BYTES_PER_PIXEL(color_depth));
   if (!bitmap->dat) {
      _AL_FREE(bitmap);
      return NULL;
   }

   bitmap->w = bitmap->cr = width;
   bitmap->h = bitmap->cb = height;
   bitmap->clip = TRUE;
   bitmap->cl = bitmap->ct = 0;
   bitmap->vtable = vtable;
   bitmap->write_bank = bitmap->read_bank = _stub_bank_switch;
   bitmap->id = 0;
   bitmap->x_ofs = 0;
   bitmap->y_ofs = 0;
   bitmap->seg = _default_ds();

   if (height > 0) {
      bitmap->line[0] = bitmap->dat;
      for (i=1; i<height; i++)
         bitmap->line[i] = bitmap->line[i-1] + pitch * BYTES_PER_PIXEL(color_depth);
   }

   /* Setup info structure to store additional information. */
   bitmap->extra = malloc(sizeof(struct BMP_EXTRA_INFO));
   if (!bitmap->extra) {
      _AL_FREE(bitmap);
      return NULL;
   }
   BMP_EXTRA(bitmap)->pitch = pitch;

   return bitmap;
}



/* psp_created_sub_bitmap:
 *  Set the needed sub bitmap info.
 */
static void psp_created_sub_bitmap(BITMAP *bmp, BITMAP *parent)
{
   if (BMP_EXTRA(parent)) {
      bmp->extra = malloc(sizeof(struct BMP_EXTRA_INFO));
      if (bmp->extra)
         BMP_EXTRA(bmp)->pitch = BMP_EXTRA(parent)->pitch;
   }
}



/* psp_destroy_bitmap:
 *  Destroys the bitmap extra info structure.
 */
int psp_destroy_bitmap(BITMAP *bitmap)
{
   _AL_FREE(bitmap->extra);

   return 0;
}



/* psp_get_gfx_safe_mode:
 *  Defines the safe graphics mode for this system.
 */
static void psp_get_gfx_safe_mode(int *driver, struct GFX_MODE *mode)
{
   *driver = GFX_PSP;
   mode->width = DEFAULT_SCREEN_WIDTH;
   mode->height = DEFAULT_SCREEN_HEIGHT;
   mode->bpp = DEFAULT_COLOR_DEPTH;
}


