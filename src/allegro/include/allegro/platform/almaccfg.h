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
 *      Configuration defines for use with MPW.
 *
 *      By Ronaldo Hideki Yamada.
 *
 *      See readme.txt for copyright information.
 */


#ifndef SCAN_DEPEND
	#include <MacTypes.h>
	#include <QuickDraw.h>
#endif
#define CALL_NOT_IN_CARBON 1

#include <FCntl.h>
#ifndef ENOSYS
#define ENOSYS ENOENT
#endif

/* describe this platform */
#define ALLEGRO_PLATFORM_STR  "mpw"
#define ALLEGRO_BIG_ENDIAN
#undef ALLEGRO_CONSOLE_OK

#define INLINE
#define ZERO_SIZE_ARRAY(type, name)             type name[64]
#define AL_CONST								const
#define AL_VAR(type, name)                      extern type name
#define AL_ARRAY(type, name)                    extern type name[]
#define AL_FUNC(type, name, args)               type name args
#define AL_PRINTFUNC(type, name, args, a, b)    AL_FUNC(type, name, args)
#define AL_METHOD(type, name, args)             type (*name) args
#define AL_FUNCPTR(type, name, args)            extern type (*name) args

#define END_OF_MAIN()				void x##_end(void) { }

#define END_OF_FUNCTION(x)			void x##_end(void) { }
#define END_OF_STATIC_FUNCTION(x)	static void x##_end(void) { }
#define LOCK_DATA(d, s)				_mac_lock((void *)d, s)
#define LOCK_CODE(c, s)				_mac_lock((void *)c, s)
#define UNLOCK_DATA(d,s)			_mac_unlock((void *)d, s)
#define LOCK_VARIABLE(x)			LOCK_DATA((void *)&x, sizeof(x))
#define LOCK_FUNCTION(x)			LOCK_CODE((void *)x, (intptr_t)x##_end - (intptr_t)x)

/* long filename status */
#define ALLEGRO_LFN  0

#define ALLEGRO_NO_STRICMP 1

#define ALLEGRO_NO_STRUPR 1

//#define ALLEGRO_NO_STRDUP 1
#ifdef __cplusplus
extern "C"
#endif
char *strdup(const char *);

#ifndef AL_INLINE
   #define AL_INLINE(type, name, args, code)    static type name args code
#endif


// gm_time has an strange return
#define gmtime localtime


#define ALLEGRO_EXTRA_HEADER     "allegro/platform/almac.h"
#define ALLEGRO_INTERNAL_HEADER  "allegro/platform/aintmac.h"
