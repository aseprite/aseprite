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
 *      QNX Photon bitmap management routines.
 *
 *      By Eric Botcazou.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintqnx.h"

#ifndef ALLEGRO_QNX
   #error Something is wrong with the makefile
#endif


/* this bitmap is guaranteed to point to the visible contents */
BITMAP *ph_frontbuffer;


/* we may have to recycle the screen surface as a video bitmap,
 * in order to be consistent with how other platforms behave
 */
static int reused_screen = FALSE;


#ifdef ALLEGRO_NO_ASM

/* ph_write_line:
 *  Line switcher for video bitmaps.
 */
unsigned long phd_write_line(BITMAP *bmp, int line)
{
   if (!(bmp->id & BMP_ID_LOCKED)) {
      bmp->id |= BMP_ID_LOCKED;
      PgWaitHWIdle();
   }
   
   return (unsigned long)(bmp->line[line]);
}



/* ph_unwrite_line:
 *  Line updater for video bitmaps.
 */
void ph_unwrite_line(BITMAP *bmp)
{
   if (bmp->id & BMP_ID_AUTOLOCK) {
      bmp->id &= ~(BMP_ID_LOCKED | BMP_ID_AUTOLOCK);
   }
}

#endif


/* ph_acquire:
 *  Bitmap locking for video bitmaps.
 */
void ph_acquire(BITMAP *bmp)
{
   if (!(bmp->id & BMP_ID_LOCKED)) {
      bmp->id |= BMP_ID_LOCKED;
      PgWaitHWIdle();
   }
}



/* ph_release:
 *  Bitmap unlocking for video bitmaps.
 */
void ph_release(BITMAP *bmp)
{
   bmp->id &= ~BMP_ID_LOCKED;
}



/* _make_video_bitmap:
 *  Helper function for wrapping up video memory in a video bitmap.
 */
static BITMAP *_make_video_bitmap(int w, int h, unsigned long addr, struct GFX_VTABLE *vtable, int bpl)
{
   int i, size;
   BITMAP *b;

   if (!vtable)
      return NULL;

   size = sizeof(BITMAP) + sizeof(char *) * h;

   b = (BITMAP *) _AL_MALLOC(size);
   if (!b)
      return NULL;

   b->w = b->cr = w;
   b->h = b->cb = h;
   b->clip = TRUE;
   b->cl = b->ct = 0;
   b->vtable = vtable;
   b->write_bank = b->read_bank = _stub_bank_switch;
   b->dat = NULL;
   b->id = BMP_ID_VIDEO;
   b->extra = NULL;
   b->x_ofs = 0;
   b->y_ofs = 0;
   b->seg = _video_ds();

   b->line[0] = (char *)addr;

   for (i = 1; i < h; i++)
      b->line[i] = b->line[i - 1] + bpl;

   return b;
}



/* make_photon_bitmap:
 *  Connects a Photon context with an Allegro bitmap.
 */
BITMAP *make_photon_bitmap(PdOffscreenContext_t *context, int w, int h, int id)
{
   struct BITMAP *bmp;
   char *addr;

   addr = PdGetOffscreenContextPtr(context);
   if (!addr)
      return NULL;

   /* create Allegro bitmap */
   bmp = _make_video_bitmap(w, h, (unsigned long)addr, &_screen_vtable, context->pitch);
   if (!bmp)
      return NULL;

   bmp->id = id;
   
#ifdef ALLEGRO_NO_ASM
   bmp->write_bank = ph_write_line;
   bmp->read_bank = ph_write_line;
#else
   bmp->write_bank = ph_write_line_asm;
   bmp->read_bank = ph_write_line_asm;
#endif

   /* setup surface info structure to store additional information */
   bmp->extra = _AL_MALLOC(sizeof(struct BMP_EXTRA_INFO));
   BMP_EXTRA(bmp)->context = context;

   return bmp;
}

   

/* destroy_photon_bitmap:
 *  Destroys a video bitmap.
 */
void destroy_photon_bitmap(BITMAP *bmp)
{
   if (bmp) {
      _AL_FREE(bmp->extra);
      _AL_FREE(bmp);
   }
}



/* qnx_ph_created_sub_bitmap:
 */
void qnx_ph_created_sub_bitmap(BITMAP *bmp, BITMAP *parent)
{
   bmp->extra = parent;
}



/* qnx_ph_create_video_bitmap:
 */
BITMAP *qnx_ph_create_video_bitmap(int width, int height)
{
   PdOffscreenContext_t *context;
   int flags = Pg_OSC_MEM_PAGE_ALIGN;
   
   if ((width == screen->w) && (height == screen->h)) {
      if (!reused_screen) {
         reused_screen = TRUE;
         return screen;
      }
      
      flags |= Pg_OSC_CRTC_SAFE;
   }
   
   context = PdCreateOffscreenContext(0, width, height, flags);
      
   if (!context)
      return NULL;

   return make_photon_bitmap(context, width, height, BMP_ID_VIDEO);
}



/* qnx_ph_destroy_video_bitmap:
 */
void qnx_ph_destroy_video_bitmap(BITMAP *bmp)
{
   if (bmp == screen) {
      reused_screen = FALSE;
      return;
   }

   if (bmp == ph_frontbuffer) {
      /* in this case, 'bmp' points to the visible contents
       * but 'screen' doesn't, so we first invert that
       */
      qnx_ph_show_video_bitmap(screen);
   }

   PhDCRelease(BMP_EXTRA(bmp)->context);
   destroy_photon_bitmap(bmp);
}



/* swap_video_bitmap:
 *  Worker function for Photon page flipping.
 */
static int swap_video_bitmap(BITMAP *bmp, int vsync)
{
   if (PgSwapDisplay(BMP_EXTRA(bmp)->context, vsync ? Pg_SWAP_VSYNC : 0) != 0)
      return -1;
 
   PgFlush();
                
   /* ph_frontbuffer must keep track of the forefront bitmap */
   ph_frontbuffer = bmp;
   
   return 0;
}
   


/* qnx_ph_show_video_bitmap:
 */
int qnx_ph_show_video_bitmap(BITMAP *bmp)
{
   return swap_video_bitmap(bmp, _wait_for_vsync);
}



/* qnx_ph_request_video_bitmap:
 */
int qnx_ph_request_video_bitmap(BITMAP *bmp)
{
   return swap_video_bitmap(bmp, FALSE);
}
