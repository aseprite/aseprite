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
 *      Fixed point math routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_FMATH_H
#define ALLEGRO_FMATH_H

#include "base.h"
#include "fixed.h"

#ifdef __cplusplus
   extern "C" {
#endif

AL_FUNC(fixed, fixsqrt, (fixed x));
AL_FUNC(fixed, fixhypot, (fixed x, fixed y));
AL_FUNC(fixed, fixatan, (fixed x));
AL_FUNC(fixed, fixatan2, (fixed y, fixed x));

AL_ARRAY(fixed, _cos_tbl);
AL_ARRAY(fixed, _tan_tbl);
AL_ARRAY(fixed, _acos_tbl);

#ifdef __cplusplus
   }
#endif

#include "inline/fmaths.inl"

#endif          /* ifndef ALLEGRO_FMATH_H */


