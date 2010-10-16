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
 *      Configuration defines for use with djgpp.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef SCAN_DEPEND
   #include <pc.h>
   #include <dir.h>
   #include <dpmi.h>
   #include <go32.h>
   #include <fcntl.h>
   #include <unistd.h>
   #include <sys/farptr.h>
#endif


/* describe this platform */
#define ALLEGRO_PLATFORM_STR  "djgpp"
#define ALLEGRO_DOS
#define ALLEGRO_I386
#define ALLEGRO_LITTLE_ENDIAN
#define ALLEGRO_GUESS_INTTYPES_OK
   /* inttypes.h and stdint.h not available in djgpp 2.02 */
#define ALLEGRO_CONSOLE_OK
#define ALLEGRO_VRAM_SINGLE_SURFACE
#define ALLEGRO_USE_CONSTRUCTOR


#ifdef __cplusplus
extern "C" {
#endif

/* memory locking macros */
void _unlock_dpmi_data(void *addr, int size);

#define END_OF_FUNCTION(x)          void x##_end(void) { }
#define END_OF_STATIC_FUNCTION(x)   static void x##_end(void) { }
#define LOCK_DATA(d, s)             _go32_dpmi_lock_data((void *)d, s)
#define LOCK_CODE(c, s)             _go32_dpmi_lock_code((void *)c, s)
#define UNLOCK_DATA(d,s)            _unlock_dpmi_data((void *)d, s)
#define LOCK_VARIABLE(x)            LOCK_DATA((void *)&x, sizeof(x))
#define LOCK_FUNCTION(x)            LOCK_CODE((void *)x, (intptr_t)x##_end - (intptr_t)x)


/* long filename status */
#ifdef _USE_LFN
   #define ALLEGRO_LFN  _USE_LFN
#else
   #define ALLEGRO_LFN  0
#endif


/* selector for video memory bitmaps */
#define _video_ds()  _dos_ds


/* helpers for talking to video memory */
#define bmp_select(bmp)             _farsetsel((bmp)->seg)

#define bmp_write8(addr, c)         _farnspokeb(addr, c)
#define bmp_write15(addr, c)        _farnspokew(addr, c)
#define bmp_write16(addr, c)        _farnspokew(addr, c)
#define bmp_write24(addr, c)        ({ _farnspokew(addr, c&0xFFFF); \
                                       _farnspokeb(addr+2, c>>16); })
#define bmp_write32(addr, c)        _farnspokel(addr, c)

#define bmp_read8(addr)             _farnspeekb(addr)
#define bmp_read15(addr)            _farnspeekw(addr)
#define bmp_read16(addr)            _farnspeekw(addr)
#define bmp_read32(addr)            _farnspeekl(addr)
#define bmp_read24(addr)            (_farnspeekl(addr) & 0xFFFFFF)

#ifdef __cplusplus
}
#endif


/* describe the asm syntax for this platform */
#define ALLEGRO_ASM_PREFIX    "_"
#define ALLEGRO_ASM_USE_FS


/* arrange for other headers to be included later on */
#define ALLEGRO_EXTRA_HEADER     "allegro/platform/aldos.h"
#define ALLEGRO_INTERNAL_HEADER  "allegro/platform/aintdos.h"
#define ALLEGRO_ASMCAPA_HEADER   "obj/djgpp/asmcapa.h"
