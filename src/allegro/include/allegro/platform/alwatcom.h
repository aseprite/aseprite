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
 *      Configuration defines for use with Watcom.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */

#ifndef SCAN_DEPEND
   #ifndef __SW_3S
      #error Allegro only supports stack based calling convention
   #endif

   #ifndef __SW_S
      #error Stack overflow checking must be disabled
   #endif

   #include <io.h>
   #include <i86.h>
   #include <conio.h>
   #include <fcntl.h>
   #include <direct.h>
   #include <malloc.h>
#endif


#pragma disable_message (120 201 202)


/* these are available in OpenWatcom 1.3 (12.3) */
#if __WATCOMC__ >= 1230
   #define ALLEGRO_HAVE_INTTYPES_H	1
   #define ALLEGRO_HAVE_STDINT_H	1
#else
   #define ALLEGRO_GUESS_INTTYPES_OK
#endif


/* describe this platform */
#define ALLEGRO_PLATFORM_STR  "Watcom"
#define ALLEGRO_DOS
#define ALLEGRO_I386
#define ALLEGRO_LITTLE_ENDIAN
#define ALLEGRO_CONSOLE_OK
#define ALLEGRO_VRAM_SINGLE_SURFACE

#define ALLEGRO_LFN  0

#define LONG_LONG    long long
#ifdef ALLEGRO_GUESS_INTTYPES_OK
   #define int64_t   signed long long
   #define uint64_t  unsigned long long
#endif

#if __WATCOMC__ >= 1100
   #define ALLEGRO_MMX
#endif

#if __WATCOMC__ >= 1200   /* Open Watcom 1.0 */
   #define AL_CONST const
#endif


/* emulate some important djgpp routines */
#define inportb(port)         inp(port)
#define inportw(port)         inpw(port)
#define outportb(port, val)   outp(port, val)
#define outportw(port, val)   outpw(port, val)

#define ffblk        find_t
#define ff_name      name
#define ff_attrib    attrib
#define ff_fsize     size
#define ff_ftime     wr_time
#define ff_fdate     wr_date

#define findfirst(name, dta, attrib)   _dos_findfirst(name, attrib, dta)
#define findnext(dta)                  _dos_findnext(dta)

#define random()     rand()
#define srandom(n)   srand(n)

#define _dos_ds      _default_ds()

#define dosmemget(offset, length, buffer)    memcpy(buffer, (void *)(offset), length)
#define dosmemput(buffer, length, offset)    memcpy((void *)(offset), buffer, length)

#define __djgpp_nearptr_enable()    1
#define __djgpp_nearptr_disable()

#define __djgpp_base_address        0
#define __djgpp_conventional_base   0

#define _crt0_startup_flags         1
#define _CRT0_FLAG_NEARPTR          1


#ifdef __cplusplus
extern "C" {
#endif

typedef union __dpmi_regs
{
   struct {
      unsigned long edi, esi, ebp, res, ebx, edx, ecx, eax;
   } d;
   struct {
      unsigned short di, di_hi, si, si_hi, bp, bp_hi, res, res_hi;
      unsigned short bx, bx_hi, dx, dx_hi, cx, cx_hi, ax, ax_hi;
      unsigned short flags, es, ds, fs, gs, ip, cs, sp, ss;
   } x;
   struct {
      unsigned char edi[4], esi[4], ebp[4], res[4];
      unsigned char bl, bh, ebx_b2, ebx_b3, dl, dh, edx_b2, edx_b3;
      unsigned char cl, ch, ecx_b2, ecx_b3, al, ah, eax_b2, eax_b3;
   } h;
} __dpmi_regs;


typedef struct __dpmi_meminfo
{
   unsigned long handle;
   unsigned long size;
   unsigned long address;
} __dpmi_meminfo;


typedef struct __dpmi_free_mem_info
{
   unsigned long largest_available_free_block_in_bytes;
   unsigned long maximum_unlocked_page_allocation_in_pages;
   unsigned long maximum_locked_page_allocation_in_pages;
   unsigned long linear_address_space_size_in_pages;
   unsigned long total_number_of_unlocked_pages;
   unsigned long total_number_of_free_pages;
   unsigned long total_number_of_physical_pages;
   unsigned long free_linear_address_space_in_pages;
   unsigned long size_of_paging_file_partition_in_pages;
   unsigned long reserved[3];
} __dpmi_free_mem_info;


extern unsigned long __tb;


int __dpmi_int(int vector, __dpmi_regs *regs);
int __dpmi_allocate_dos_memory(int paragraphs, int *ret);
int __dpmi_free_dos_memory(int selector);
int __dpmi_physical_address_mapping(__dpmi_meminfo *info);
int __dpmi_free_physical_address_mapping(__dpmi_meminfo *info);
int __dpmi_lock_linear_region(__dpmi_meminfo *info);
int __dpmi_unlock_linear_region(__dpmi_meminfo *info);
int __dpmi_allocate_ldt_descriptors(int count);
int __dpmi_free_ldt_descriptor(int descriptor);
int __dpmi_get_segment_base_address(int selector, unsigned long *addr);
int __dpmi_set_segment_base_address(int selector, unsigned long address);
int __dpmi_set_segment_limit(int selector, unsigned long limit);
int __dpmi_get_free_memory_information(__dpmi_free_mem_info *info);
int __dpmi_simulate_real_mode_interrupt(int vector, __dpmi_regs *regs);
int __dpmi_simulate_real_mode_procedure_retf(__dpmi_regs *regs);
int _go32_dpmi_lock_data(void *lockaddr, unsigned long locksize);
int _go32_dpmi_lock_code(void *lockaddr, unsigned long locksize);

long _allocate_real_mode_callback(void (*handler)(__dpmi_regs *r), __dpmi_regs *regs);


/* memory locking macros */
void _unlock_dpmi_data(void *addr, int size);

#ifdef __cplusplus
}
#endif


#define END_OF_FUNCTION(x)          void x##_end(void) { }
#define END_OF_STATIC_FUNCTION(x)   static void x##_end(void) { }
#define LOCK_DATA(d, s)             _go32_dpmi_lock_data(d, s)
#define LOCK_CODE(c, s)             _go32_dpmi_lock_code(c, s)
#define UNLOCK_DATA(d,s)            _unlock_dpmi_data(d, s)
#define LOCK_VARIABLE(x)            LOCK_DATA((void *)&x, sizeof(x))
#define LOCK_FUNCTION(x)            LOCK_CODE((void *)FP_OFF(x), (long)FP_OFF(x##_end) - (long)FP_OFF(x))


/* arrange for other headers to be included later on */
#define ALLEGRO_EXTRA_HEADER     "allegro/platform/aldos.h"
#define ALLEGRO_INTERNAL_HEADER  "allegro/platform/aintdos.h"
