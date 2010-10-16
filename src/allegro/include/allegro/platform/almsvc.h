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
 *      Configuration defines for use with MSVC.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef SCAN_DEPEND
   #include <io.h>
   #include <fcntl.h>
   #include <direct.h>
   #include <malloc.h>
#endif


#pragma warning (disable: 4200 4244 4305)


/* describe this platform */
#ifdef ALLEGRO_STATICLINK
   #define ALLEGRO_PLATFORM_STR  "MSVC.s"
#else
   #define ALLEGRO_PLATFORM_STR  "MSVC"
#endif

#define ALLEGRO_WINDOWS
#define ALLEGRO_I386
#define ALLEGRO_LITTLE_ENDIAN
#define ALLEGRO_GUESS_INTTYPES_OK
#define ALLEGRO_MULTITHREADED

#ifdef ALLEGRO_USE_CONSOLE
   #define ALLEGRO_CONSOLE_OK
   #define ALLEGRO_NO_MAGIC_MAIN
#endif

#ifdef ALLEGRO_AND_MFC
   #define ALLEGRO_NO_MAGIC_MAIN
#endif


/* describe how function prototypes look to MSVC */
#if defined ALLEGRO_STATICLINK
   #define _AL_DLL
#elif defined ALLEGRO_SRC
   #define _AL_DLL   __declspec(dllexport)
#else
   #define _AL_DLL   __declspec(dllimport)
#endif

#define AL_VAR(type, name)             extern _AL_DLL type name
#define AL_ARRAY(type, name)           extern _AL_DLL type name[]
#define AL_FUNC(type, name, args)      _AL_DLL type __cdecl name args
#define AL_METHOD(type, name, args)    type (__cdecl *name) args
#define AL_FUNCPTR(type, name, args)   extern _AL_DLL type (__cdecl *name) args

#ifdef AL_INLINE
   #define END_OF_INLINE(name)         void *_force_instantiate_##name = name;
#else
   #define END_OF_INLINE(name)
#endif

#undef AL_INLINE

#define AL_INLINE(type, name, args, code)    __inline _AL_DLL type __cdecl name args code END_OF_INLINE(name)

#define INLINE       __inline

#define LONG_LONG    __int64
#define int64_t      signed __int64
#define uint64_t     unsigned __int64

#define AL_CONST     const


/* describe the asm syntax for this platform */
#define ALLEGRO_ASM_PREFIX    "_"


/* life would be so easy if compilers would all use the same names! */
#if (!defined S_IRUSR) && (!defined SCAN_DEPEND)
   #define S_IRUSR   S_IREAD
   #define S_IWUSR   S_IWRITE
#endif


/* arrange for other headers to be included later on */
#define ALLEGRO_EXTRA_HEADER     "allegro/platform/alwin.h"
#define ALLEGRO_INTERNAL_HEADER  "allegro/platform/aintwin.h"
#define ALLEGRO_ASMCAPA_HEADER   "obj/msvc/asmcapa.h"
