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
 *      Debug facilities.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_DEBUG_H
#define ALLEGRO_DEBUG_H

#include "base.h"

#ifdef __cplusplus
   extern "C" {
#endif

AL_FUNC(void, al_assert, (AL_CONST char *file, int linenr));
AL_PRINTFUNC(void, al_trace, (AL_CONST char *msg, ...), 1, 2);

AL_FUNC(void, register_assert_handler, (AL_METHOD(int, handler, (AL_CONST char *msg))));
AL_FUNC(void, register_trace_handler, (AL_METHOD(int, handler, (AL_CONST char *msg))));


#ifdef DEBUGMODE
   #define ASSERT(condition)     { if (!(condition)) al_assert(__FILE__, __LINE__); }
   #define TRACE                 al_trace
#else
   #define ASSERT(condition)
   #define TRACE                 1 ? (void) 0 : al_trace
#endif

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_DEBUG_H */


