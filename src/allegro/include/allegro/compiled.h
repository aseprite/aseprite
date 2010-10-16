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
 *      Compiled sprites.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_COMPILED_H
#define ALLEGRO_COMPILED_H

#include "base.h"

#ifdef __cplusplus
   extern "C" {
#endif

struct BITMAP;

#if (defined ALLEGRO_I386) && (!defined ALLEGRO_NO_ASM)

/* compiled sprite structure */
typedef struct COMPILED_SPRITE
{
   short planar;                    /* set if it's a planar (mode-X) sprite */
   short color_depth;               /* color depth of the image */
   short w, h;                      /* size of the sprite */
   struct {
      void *draw;                   /* routines to draw the image */
      int len;                      /* length of the drawing functions */
   } proc[4];
} COMPILED_SPRITE;

#else

/* emulate compiled sprites using RLE on other platforms */
struct RLE_SPRITE;
typedef struct RLE_SPRITE COMPILED_SPRITE;

#endif

AL_FUNC(COMPILED_SPRITE *, get_compiled_sprite, (struct BITMAP *bitmap, int planar));
AL_FUNC(void, destroy_compiled_sprite, (COMPILED_SPRITE *sprite));
AL_FUNC(void, draw_compiled_sprite, (struct BITMAP *bmp, AL_CONST COMPILED_SPRITE *sprite, int x, int y));

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_COMPILED_H */


