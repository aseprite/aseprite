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
 *      RLE sprites.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_RLE_H
#define ALLEGRO_RLE_H

#include "base.h"
#include "gfx.h"

#ifdef __cplusplus
   extern "C" {
#endif

typedef struct RLE_SPRITE           /* a RLE compressed sprite */
{
   int w, h;                        /* width and height in pixels */
   int color_depth;                 /* color depth of the image */
   int size;                        /* size of sprite data in bytes */
   ZERO_SIZE_ARRAY(signed char, dat);
} RLE_SPRITE;


AL_FUNC(RLE_SPRITE *, get_rle_sprite, (struct BITMAP *bitmap));
AL_FUNC(void, destroy_rle_sprite, (RLE_SPRITE *sprite));

#ifdef __cplusplus
   }
#endif

#include "inline/rle.inl"

#endif          /* ifndef ALLEGRO_RLE_H */


