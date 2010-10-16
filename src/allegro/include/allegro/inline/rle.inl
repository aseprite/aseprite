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
 *      RLE sprite inline functions (generic C).
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_RLE_INL
#define ALLEGRO_RLE_INL

#include "allegro/debug.h"

#ifdef __cplusplus
   extern "C" {
#endif


AL_INLINE(void, draw_rle_sprite, (struct BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y),
{
   ASSERT(bmp);
   ASSERT(sprite);
   ASSERT(bmp->vtable->color_depth == sprite->color_depth);

   bmp->vtable->draw_rle_sprite(bmp, sprite, x, y);
})


AL_INLINE(void, draw_trans_rle_sprite, (struct BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y),
{
   ASSERT(bmp);
   ASSERT(sprite);

   if (sprite->color_depth == 32) {
      ASSERT(bmp->vtable->draw_trans_rgba_rle_sprite);
      bmp->vtable->draw_trans_rgba_rle_sprite(bmp, sprite, x, y);
   }
   else {
      ASSERT(bmp->vtable->color_depth == sprite->color_depth);
      bmp->vtable->draw_trans_rle_sprite(bmp, sprite, x, y);
   }
})


AL_INLINE(void, draw_lit_rle_sprite, (struct BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y, int color),
{
   ASSERT(bmp);
   ASSERT(sprite);
   ASSERT(bmp->vtable->color_depth == sprite->color_depth);

   bmp->vtable->draw_lit_rle_sprite(bmp, sprite, x, y, color);
})

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_RLE_INL */


