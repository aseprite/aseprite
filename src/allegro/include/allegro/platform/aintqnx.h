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
 *      Internal header for the QNX Allegro library.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


#ifndef AINTQNX_H
#define AINTQNX_H

#include "allegro/platform/aintunix.h"

#ifndef SCAN_DEPEND
   #include <pthread.h>
   #include <Ph.h>
   #include <Pt.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif


/* from qphaccel.c */
AL_FUNC(void, enable_acceleration, (GFX_DRIVER * drv));
AL_FUNC(void, enable_triple_buffering, (GFX_DRIVER *drv));


/* from qphbmp.c */
typedef struct BMP_EXTRA_INFO {
   PdOffscreenContext_t *context;
} BMP_EXTRA_INFO;

#define BMP_EXTRA(bmp) ((BMP_EXTRA_INFO *)(bmp->extra))

AL_VAR(BITMAP *, ph_frontbuffer);

AL_FUNC(uintptr_t, ph_write_line, (BITMAP *bmp, int lyne));
AL_FUNC(uintptr_t, ph_write_line_asm, (BITMAP *bmp, int lyne));
AL_FUNC(void, ph_unwrite_line, (BITMAP *bmp, int lyne));
AL_FUNC(void, ph_unwrite_line_asm, (BITMAP *bmp, int lyne));
AL_FUNC(void, ph_acquire, (BITMAP *bmp));
AL_FUNC(void, ph_release, (BITMAP *bmp));
AL_FUNC(BITMAP *, make_photon_bitmap, (PdOffscreenContext_t *context, int w, int h, int id));
AL_FUNC(void, destroy_photon_bitmap, (BITMAP *bmp));
AL_FUNC(void, qnx_ph_created_sub_bitmap, (BITMAP *bmp, BITMAP *parent));
AL_FUNC(BITMAP *, qnx_ph_create_video_bitmap, (int width, int height));
AL_FUNC(void, qnx_ph_destroy_video_bitmap, (BITMAP *bmp));
AL_FUNC(int, qnx_ph_show_video_bitmap, (BITMAP *bmp));
AL_FUNC(int, qnx_ph_request_video_bitmap, (BITMAP *bmp));


/* from qphoton.c */
#define PH_GFX_NONE       0
#define PH_GFX_WINDOW     1
#define PH_GFX_DIRECT     2
#define PH_GFX_OVERLAY    3

AL_VAR(int, ph_gfx_mode);
AL_VAR(PgVideoModeInfo_t, ph_gfx_mode_info);
AL_ARRAY(PgColor_t, ph_palette);

AL_FUNC(void, setup_direct_shifts, (void));
AL_FUNC(void, setup_driver, (GFX_DRIVER *drv, int w, int h, int color_depth));


/* from qphwin.c */
AL_VAR(BITMAP *, pseudo_screen);

AL_FUNCPTR(void, ph_update_window, (PhRect_t* rect));


/* from qsystem.c */
AL_VAR(PtWidget_t, *ph_window);
AL_VAR(pthread_mutex_t, qnx_event_mutex);
AL_VAR(pthread_mutex_t, qnx_gfx_mutex);


/* from qkeydrv.c */
AL_FUNC(void, qnx_keyboard_handler, (int, int));
AL_FUNC(void, qnx_keyboard_focused, (int, int));


/* from qmouse.c */
AL_VAR(int, qnx_mouse_warped);
AL_FUNC(void, qnx_mouse_handler, (int, int, int, int));


/* A very strange thing: PgWaitHWIdle() cannot be found in any system
 * header file, but it is explained in the QNX docs, and it actually
 * exists in the Photon library... So until QNX fixes the missing declaration,
 * we will declare it here.
 */
int PgWaitHWIdle(void);


#ifdef __cplusplus
}
#endif

#endif
