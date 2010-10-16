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
 *      Graphics inline functions (generic C).
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_GFX_INL
#define ALLEGRO_GFX_INL

#include "allegro/debug.h"

#define ALLEGRO_IMPORT_GFX_ASM
#include "asm.inl"
#undef ALLEGRO_IMPORT_GFX_ASM

#ifdef __cplusplus
   extern "C" {
#endif


#ifdef ALLEGRO_NO_ASM

   /* use generic C versions */

AL_INLINE(int, _default_ds, (void),
{
   return 0;
})

#ifdef ALLEGRO_BCC32

   /* BCC32 is a somewhat unusual platform because it mixes a MSVC/MinGW generated DLL
    * (for which ALLEGRO_NO_ASM is not defined) with Borland C++ compiled programs for
    * which ALLEGRO_NO_ASM is defined. As a result, Borland C++ compiled programs can't
    * use the inlined version of bmp_write_line(), bmp_read_line() and bmp_unwrite_line()
    * because the write_bank() and read_bank() methods of the BITMAP class don't expect
    * the same calling convention on both sides.
    */

AL_FUNC(uintptr_t, bmp_write_line, (BITMAP *bmp, int lyne));
AL_FUNC(uintptr_t, bmp_read_line, (BITMAP *bmp, int lyne));
AL_FUNC(void, bmp_unwrite_line, (BITMAP *bmp));

#else

typedef AL_METHOD(uintptr_t, _BMP_BANK_SWITCHER, (BITMAP *bmp, int lyne));
typedef AL_METHOD(void, _BMP_UNBANK_SWITCHER, (BITMAP *bmp));


AL_INLINE(uintptr_t, bmp_write_line, (BITMAP *bmp, int lyne),
{
   _BMP_BANK_SWITCHER switcher = (_BMP_BANK_SWITCHER)bmp->write_bank;
   return switcher(bmp, lyne);
})


AL_INLINE(uintptr_t, bmp_read_line, (BITMAP *bmp, int lyne),
{
   _BMP_BANK_SWITCHER switcher = (_BMP_BANK_SWITCHER)bmp->read_bank;
   return switcher(bmp, lyne);
})


AL_INLINE(void, bmp_unwrite_line, (BITMAP *bmp),
{
   _BMP_UNBANK_SWITCHER switcher = (_BMP_UNBANK_SWITCHER)bmp->vtable->unwrite_bank;
   switcher(bmp);
})

#endif      /* defined ALLEGRO_BCC32 */

#endif      /* C vs. inline asm */


AL_INLINE(int, is_windowed_mode, (void),
{
   ASSERT (gfx_driver);

   return gfx_driver->windowed;
})



AL_INLINE(void, clear_to_color, (BITMAP *bitmap, int color),
{
   ASSERT(bitmap);

   bitmap->vtable->clear_to_color(bitmap, color);
})


AL_INLINE(int, bitmap_color_depth, (BITMAP *bmp),
{
   ASSERT(bmp);

   return bmp->vtable->color_depth;
})


AL_INLINE(int, bitmap_mask_color, (BITMAP *bmp),
{
   ASSERT(bmp);

   return bmp->vtable->mask_color;
})


AL_INLINE(int, is_same_bitmap, (BITMAP *bmp1, BITMAP *bmp2),
{
   unsigned long m1;
   unsigned long m2;

   if ((!bmp1) || (!bmp2))
      return FALSE;

   if (bmp1 == bmp2)
      return TRUE;

   m1 = bmp1->id & BMP_ID_MASK;
   m2 = bmp2->id & BMP_ID_MASK;

   return ((m1) && (m1 == m2));
})


AL_INLINE(int, is_linear_bitmap, (BITMAP *bmp),
{
   ASSERT(bmp);

   return (bmp->id & BMP_ID_PLANAR) == 0;
})


AL_INLINE(int, is_planar_bitmap, (BITMAP *bmp),
{
   ASSERT(bmp);

   return (bmp->id & BMP_ID_PLANAR) != 0;
})


AL_INLINE(int, is_memory_bitmap, (BITMAP *bmp),
{
   ASSERT(bmp);

   return (bmp->id & (BMP_ID_VIDEO | BMP_ID_SYSTEM)) == 0;
})


AL_INLINE(int, is_screen_bitmap, (BITMAP *bmp),
{
   ASSERT(bmp);

   return is_same_bitmap(bmp, screen);
})


AL_INLINE(int, is_video_bitmap, (BITMAP *bmp),
{
   ASSERT(bmp);

   return (bmp->id & BMP_ID_VIDEO) != 0;
})


AL_INLINE(int, is_system_bitmap, (BITMAP *bmp),
{
   ASSERT(bmp);

   return (bmp->id & BMP_ID_SYSTEM) != 0;
})


AL_INLINE(int, is_sub_bitmap, (BITMAP *bmp),
{
   ASSERT(bmp);

   return (bmp->id & BMP_ID_SUB) != 0;
})


#ifdef ALLEGRO_MPW

   #define acquire_bitmap(bmp)
   #define release_bitmap(bmp)
   #define acquire_screen()
   #define release_screen()

#else

AL_INLINE(void, acquire_bitmap, (BITMAP *bmp),
{
   ASSERT(bmp);

   if (bmp->vtable->acquire)
      bmp->vtable->acquire(bmp);
})


AL_INLINE(void, release_bitmap, (BITMAP *bmp),
{
   ASSERT(bmp);

   if (bmp->vtable->release)
      bmp->vtable->release(bmp);
})


AL_INLINE(void, acquire_screen, (void),
{
   acquire_bitmap(screen);
})


AL_INLINE(void, release_screen, (void),
{
   release_bitmap(screen);
})

#endif


AL_INLINE(int, is_inside_bitmap, (BITMAP *bmp, int x, int y, int clip),
{
   ASSERT(bmp);

   if (clip) {
      if (bmp->clip)
         /* internal clipping is inclusive-exclusive */
         return (x >= bmp->cl) && (y >= bmp->ct) && (x < bmp->cr) && (y < bmp->cb);
      else
         return TRUE;
   }
   else
      /* bitmap dimensions are always non-negative */
      return (unsigned int)x < (unsigned int)bmp->w && (unsigned int)y < (unsigned int)bmp->h;
})


AL_INLINE(void, get_clip_rect, (BITMAP *bitmap, int *x1, int *y_1, int *x2, int *y2),
{
   ASSERT(bitmap);

   /* internal clipping is inclusive-exclusive */
   *x1 = bitmap->cl;
   *y_1 = bitmap->ct;
   *x2 = bitmap->cr-1;
   *y2 = bitmap->cb-1;
})

AL_INLINE(void, set_clip_state, (BITMAP *bitmap, int state),
{
   ASSERT(bitmap);

   bitmap->clip = state;
})

AL_INLINE(int, get_clip_state, (BITMAP *bitmap),
{
   ASSERT(bitmap);

   return bitmap->clip;
})


#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_GFX_INL */


