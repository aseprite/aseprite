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
 *      A header file to get definitions of uint*_t and int*_t.
 *
 *      By Peter Wang.
 * 
 *      See readme.txt for copyright information.
 */

#ifndef ASTDINT_H
#define ASTDINT_H

/* Please only include this file from include/allegro/internal/alconfig.h
 * and don't add more than inttypes.h/stdint.h emulation here.  Thanks.
 */



#if defined ALLEGRO_HAVE_INTTYPES_H
   #include <inttypes.h>
#elif defined ALLEGRO_HAVE_STDINT_H
   #include <stdint.h>
#elif defined ALLEGRO_I386 && defined ALLEGRO_LITTLE_ENDIAN
   #ifndef ALLEGRO_GUESS_INTTYPES_OK
      #warning Guessing the definitions of fixed-width integer types.
   #endif
   #define int8_t       signed char
   #define uint8_t      unsigned char
   #define int16_t      signed short
   #define uint16_t     unsigned short
   #define int32_t      signed int
   #define uint32_t     unsigned int
   #define intptr_t     int32_t
   #define uintptr_t    uint32_t
#else
   #error I dunno how to get the definitions of fixed-width integer types on your platform.  Please report this to your friendly Allegro developer.
#endif



#endif /* ifndef ASTDINT_H */
