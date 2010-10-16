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
 *      Configuration defines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


/* which color depths to include? */
#define ALLEGRO_COLOR8
#define ALLEGRO_COLOR16
#define ALLEGRO_COLOR24
#define ALLEGRO_COLOR32


/* for backward compatibility */
#ifdef USE_CONSOLE
   #define ALLEGRO_NO_MAGIC_MAIN
   #define ALLEGRO_USE_CONSOLE
#endif


/* include platform-specific stuff */
#ifndef SCAN_EXPORT
   #ifndef SCAN_DEPEND
      #include "allegro/platform/alplatf.h"
   #endif

   #if defined ALLEGRO_DJGPP
      #include "allegro/platform/aldjgpp.h"
   #elif defined ALLEGRO_WATCOM
      #include "allegro/platform/alwatcom.h"
   #elif defined ALLEGRO_MINGW32
      #include "allegro/platform/almngw32.h"
   #elif defined ALLEGRO_DMC
      #include "allegro/platform/aldmc.h"
   #elif defined ALLEGRO_BCC32
      #include "allegro/platform/albcc32.h"
   #elif defined ALLEGRO_MSVC
      #include "allegro/platform/almsvc.h"
   #elif defined ALLEGRO_HAIKU
      #include "allegro/platform/albecfg.h"
   #elif defined ALLEGRO_BEOS
      #include "allegro/platform/albecfg.h"
   #elif defined ALLEGRO_MPW
      #include "allegro/platform/almaccfg.h"
   #elif defined ALLEGRO_MACOSX
      #include "allegro/platform/alosxcfg.h"
   #elif defined ALLEGRO_QNX
      #include "allegro/platform/alqnxcfg.h"
   #elif defined ALLEGRO_UNIX
      #include "allegro/platform/alucfg.h"
   #elif defined ALLEGRO_PSP
      #include "allegro/platform/alpspcfg.h"
   #else
      #error platform not supported
   #endif

   #ifndef SCAN_DEPEND
      #include "allegro/platform/astdint.h"
   #endif
#endif


/* special definitions for the GCC compiler */
#ifdef __GNUC__
   #define ALLEGRO_GCC

   #ifndef AL_INLINE
      #ifdef __cplusplus
         #define AL_INLINE(type, name, args, code)    \
            static inline type name args;             \
            static inline type name args code
      /* Needed if this header is included by C99 user code, as
       * "extern __inline__" is defined differently in C99 (it exports
       * a new global function symbol).
       */
      #elif __GNUC_STDC_INLINE__
         #define AL_INLINE(type, name, args, code)    \
            extern __inline__ __attribute__((__gnu_inline__)) type name args;         \
            extern __inline__ __attribute__((__gnu_inline__)) type name args code
      #else
         #define AL_INLINE(type, name, args, code)    \
            extern __inline__ type name args;         \
            extern __inline__ type name args code
      #endif
   #endif

   #define AL_PRINTFUNC(type, name, args, a, b)    AL_FUNC(type, name, args) __attribute__ ((format (printf, a, b)))

   #ifndef INLINE
      #define INLINE          __inline__
   #endif

   #if __GNUC__ >= 3
      /* SET: According to gcc volatile is ignored for a return type.
       * I think the code should just ensure that inportb is declared as an
       * __asm__ __volatile__ macro. If that's the case the extra volatile
       * doesn't have any sense.
       */
      #define RET_VOLATILE
   #else
      #define RET_VOLATILE    volatile
   #endif

   #ifndef ZERO_SIZE_ARRAY
      #if __GNUC__ < 3
         #define ZERO_SIZE_ARRAY(type, name)  __extension__ type name[0]
      #else
         #define ZERO_SIZE_ARRAY(type, name)  type name[] /* ISO C99 flexible array members */
      #endif
   #endif
   
   #ifndef LONG_LONG
      #define LONG_LONG       long long
      #ifdef ALLEGRO_GUESS_INTTYPES_OK
         #define int64_t      signed long long
         #define uint64_t     unsigned long long
      #endif
   #endif

   #ifdef __i386__
      #define ALLEGRO_I386
      #ifndef ALLEGRO_NO_ASM
         #define _AL_SINCOS(x, s, c)  __asm__ ("fsincos" : "=t" (c), "=u" (s) : "0" (x))
      #endif
   #endif

   #ifdef __amd64__
      #define ALLEGRO_AMD64
      #ifndef ALLEGRO_NO_ASM
         #define _AL_SINCOS(x, s, c)  __asm__ ("fsincos" : "=t" (c), "=u" (s) : "0" (x))
      #endif
   #endif
   
   #ifdef __arm__
      #define ALLEGRO_ARM
   #endif

   #ifndef AL_CONST
      #define AL_CONST     const
   #endif

   #ifndef AL_FUNC_DEPRECATED
      #if (__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1))
         #define AL_FUNC_DEPRECATED(type, name, args)              AL_FUNC(__attribute__ ((deprecated)) type, name, args)
         #define AL_PRINTFUNC_DEPRECATED(type, name, args, a, b)   AL_PRINTFUNC(__attribute__ ((deprecated)) type, name, args, a, b)
         #define AL_INLINE_DEPRECATED(type, name, args, code)      AL_INLINE(__attribute__ ((deprecated)) type, name, args, code)
      #endif
   #endif

   #ifndef AL_ALIAS
      #define AL_ALIAS(DECL, CALL)                      \
      static __attribute__((unused)) __inline__ DECL    \
      {                                                 \
         return CALL;                                   \
      }
   #endif

   #ifndef AL_ALIAS_VOID_RET
      #define AL_ALIAS_VOID_RET(DECL, CALL)                  \
      static __attribute__((unused)) __inline__ void DECL    \
      {                                                      \
         CALL;                                               \
      }
   #endif
#endif


/* use constructor functions, if supported */
#ifdef ALLEGRO_USE_CONSTRUCTOR
   #define CONSTRUCTOR_FUNCTION(func)              func __attribute__ ((constructor))
   #define DESTRUCTOR_FUNCTION(func)               func __attribute__ ((destructor))
#endif


/* the rest of this file fills in some default definitions of language
 * features and helper functions, which are conditionalised so they will
 * only be included if none of the above headers defined custom versions.
 */

#ifndef _AL_SINCOS
   #define _AL_SINCOS(x, s, c)  do { (c) = cos(x); (s) = sin(x); } while (0)
#endif

#ifndef INLINE
   #define INLINE
#endif

#ifndef RET_VOLATILE
   #define RET_VOLATILE   volatile
#endif

#ifndef ZERO_SIZE_ARRAY
   #define ZERO_SIZE_ARRAY(type, name)             type name[]
#endif

#ifndef AL_CONST
   #define AL_CONST
#endif

#ifndef AL_VAR
   #define AL_VAR(type, name)                      extern type name
#endif

#ifndef AL_ARRAY
   #define AL_ARRAY(type, name)                    extern type name[]
#endif

#ifndef AL_FUNC
   #define AL_FUNC(type, name, args)               type name args
#endif

#ifndef AL_PRINTFUNC
   #define AL_PRINTFUNC(type, name, args, a, b)    AL_FUNC(type, name, args)
#endif

#ifndef AL_METHOD
   #define AL_METHOD(type, name, args)             type (*name) args
#endif

#ifndef AL_FUNCPTR
   #define AL_FUNCPTR(type, name, args)            extern type (*name) args
#endif

#ifndef AL_FUNCPTRARRAY
   #define AL_FUNCPTRARRAY(type, name, args)       extern type (*name[]) args
#endif

#ifndef AL_INLINE
   #define AL_INLINE(type, name, args, code)       type name args;
#endif

#ifndef AL_FUNC_DEPRECATED
   #define AL_FUNC_DEPRECATED(type, name, args)              AL_FUNC(type, name, args)
   #define AL_PRINTFUNC_DEPRECATED(type, name, args, a, b)   AL_PRINTFUNC(type, name, args, a, b)
   #define AL_INLINE_DEPRECATED(type, name, args, code)      AL_INLINE(type, name, args, code)
#endif

#ifndef AL_ALIAS
   #define AL_ALIAS(DECL, CALL)              \
   static INLINE DECL                        \
   {                                         \
      return CALL;                           \
   }
#endif

#ifndef AL_ALIAS_VOID_RET
   #define AL_ALIAS_VOID_RET(DECL, CALL)     \
   static INLINE void DECL                   \
   {                                         \
      CALL;                                  \
   }
#endif

#ifndef END_OF_MAIN
   #define END_OF_MAIN()
#endif


/* fill in default memory locking macros */
#ifndef END_OF_FUNCTION
   #define END_OF_FUNCTION(x)
   #define END_OF_STATIC_FUNCTION(x)
   #define LOCK_DATA(d, s)
   #define LOCK_CODE(c, s)
   #define UNLOCK_DATA(d, s)
   #define LOCK_VARIABLE(x)
   #define LOCK_FUNCTION(x)
#endif


/* fill in default filename behaviour */
#ifndef ALLEGRO_LFN
   #define ALLEGRO_LFN  1
#endif

#if (defined ALLEGRO_DOS) || (defined ALLEGRO_WINDOWS)
   #define OTHER_PATH_SEPARATOR  '\\'
   #define DEVICE_SEPARATOR      ':'
#else
   #define OTHER_PATH_SEPARATOR  '/'
   #define DEVICE_SEPARATOR      '\0'
#endif


/* emulate the FA_* flags for platforms that don't already have them */
#ifndef FA_RDONLY
   #define FA_RDONLY       1
   #define FA_HIDDEN       2
   #define FA_SYSTEM       4
   #define FA_LABEL        8
   #define FA_DIREC        16
   #define FA_ARCH         32
#endif
   #define FA_NONE         0
   #define FA_ALL          (~FA_NONE)


#ifdef __cplusplus
   extern "C" {
#endif

/* emulate missing library functions */
#ifdef ALLEGRO_NO_STRICMP
   AL_FUNC(int, _alemu_stricmp, (AL_CONST char *s1, AL_CONST char *s2));
   #define stricmp _alemu_stricmp
#endif

#ifdef ALLEGRO_NO_STRLWR
   AL_FUNC(char *, _alemu_strlwr, (char *string));
   #define strlwr _alemu_strlwr
#endif

#ifdef ALLEGRO_NO_STRUPR
   AL_FUNC(char *, _alemu_strupr, (char *string));
   #define strupr _alemu_strupr
#endif

#ifdef ALLEGRO_NO_MEMCMP
   AL_FUNC(int, _alemu_memcmp, (AL_CONST void *s1, AL_CONST void *s2, size_t num));
   #define memcmp _alemu_memcmp
#endif


/* if nobody put them elsewhere, video bitmaps go in regular memory */
#ifndef _video_ds
   #define _video_ds()  _default_ds()
#endif


/* not many places actually use these, but still worth emulating */
#ifndef ALLEGRO_DJGPP
   #define _farsetsel(seg)
   #define _farnspokeb(addr, val)   (*((uint8_t  *)(addr)) = (val))
   #define _farnspokew(addr, val)   (*((uint16_t *)(addr)) = (val))
   #define _farnspokel(addr, val)   (*((uint32_t *)(addr)) = (val))
   #define _farnspeekb(addr)        (*((uint8_t  *)(addr)))
   #define _farnspeekw(addr)        (*((uint16_t *)(addr)))
   #define _farnspeekl(addr)        (*((uint32_t *)(addr)))
#endif


/* endian-independent 3-byte accessor macros */
#ifdef ALLEGRO_LITTLE_ENDIAN

   #define READ3BYTES(p)  ((*(unsigned char *)(p))               \
                           | (*((unsigned char *)(p) + 1) << 8)  \
                           | (*((unsigned char *)(p) + 2) << 16))

   #define WRITE3BYTES(p,c)  ((*(unsigned char *)(p) = (c)),             \
                              (*((unsigned char *)(p) + 1) = (c) >> 8),  \
                              (*((unsigned char *)(p) + 2) = (c) >> 16))

#elif defined ALLEGRO_BIG_ENDIAN

   #define READ3BYTES(p)  ((*(unsigned char *)(p) << 16)         \
                           | (*((unsigned char *)(p) + 1) << 8)  \
                           | (*((unsigned char *)(p) + 2)))

   #define WRITE3BYTES(p,c)  ((*(unsigned char *)(p) = (c) >> 16),       \
                              (*((unsigned char *)(p) + 1) = (c) >> 8),  \
                              (*((unsigned char *)(p) + 2) = (c)))

#elif defined SCAN_DEPEND

   #define READ3BYTES(p)
   #define WRITE3BYTES(p,c)

#else
   #error endianess not defined
#endif


/* generic versions of the video memory access helpers */
#ifndef bmp_select
   #define bmp_select(bmp)
#endif

#ifndef bmp_write8
   #define bmp_write8(addr, c)         (*((uint8_t  *)(addr)) = (c))
   #define bmp_write15(addr, c)        (*((uint16_t *)(addr)) = (c))
   #define bmp_write16(addr, c)        (*((uint16_t *)(addr)) = (c))
   #define bmp_write32(addr, c)        (*((uint32_t *)(addr)) = (c))

   #define bmp_read8(addr)             (*((uint8_t  *)(addr)))
   #define bmp_read15(addr)            (*((uint16_t *)(addr)))
   #define bmp_read16(addr)            (*((uint16_t *)(addr)))
   #define bmp_read32(addr)            (*((uint32_t *)(addr)))

   AL_INLINE(int, bmp_read24, (uintptr_t addr),
   {
      unsigned char *p = (unsigned char *)addr;
      int c;

      c = READ3BYTES(p);

      return c;
   })

   AL_INLINE(void, bmp_write24, (uintptr_t addr, int c),
   {
      unsigned char *p = (unsigned char *)addr;

      WRITE3BYTES(p, c);
   })

#endif


/* default random function definition */
#ifndef AL_RAND
   #define AL_RAND()       (rand())
#endif

#ifdef __cplusplus
   }
#endif


/* parameters for the color conversion code */
#if (defined ALLEGRO_WINDOWS) || (defined ALLEGRO_QNX)
   #define ALLEGRO_COLORCONV_ALIGNED_WIDTH
   #define ALLEGRO_NO_COLORCOPY
#endif

