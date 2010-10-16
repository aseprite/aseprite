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
 *      Fixed point type.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_FIXED_H
#define ALLEGRO_FIXED_H

#include "base.h"

#ifdef __cplusplus
   extern "C" {
#endif

typedef int32_t fixed;

AL_VAR(AL_CONST fixed, fixtorad_r);
AL_VAR(AL_CONST fixed, radtofix_r);

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_FIXED_H */


