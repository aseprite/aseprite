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
 *      Imports asm definitions of various inline functions.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */

#ifdef __cplusplus
   extern "C" {
#endif


#ifndef ALLEGRO_NO_ASM

   /* asm not supported */
   #define ALLEGRO_NO_ASM

#endif

/* Define ALLEGRO_USE_C for backwards compatibility. It should not be used
 * anywhere else in the sources for now.
 */
#define ALLEGRO_USE_C

#ifdef __cplusplus
   }
#endif
