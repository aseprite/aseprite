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
 *      Helper routines for the VBE/AF driver. This file is in charge of
 *      filling in the libc and pmode function export structures used by 
 *      the FreeBE/AF extensions, which is mainly needed for compatibility 
 *      with the SciTech Nucleus drivers (see the comments at at the top 
 *      of vbeaf.c).
 *
 *      This file is based on the SciTech PM/Lite library API.
 *
 *      Someday this might want to work on Linux, which is why it lives
 *      in the misc directory, but it isn't going to be much fun trying
 *      to make that happen :-)
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <time.h>
#include <stdio.h>
#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"

#ifdef ALLEGRO_INTERNAL_HEADER
   #include ALLEGRO_INTERNAL_HEADER
#endif

#ifndef SCAN_DEPEND
   #ifdef ALLEGRO_DOS
      #include <dos.h>
      #include <conio.h>
   #endif

   #ifdef ALLEGRO_DJGPP
      #include <crt0.h>
      #include <sys/nearptr.h>
   #endif
#endif


#define BUFFER_SIZE  256


/* FAFEXT_LIBC extension structure */
typedef struct LIBC_DATA
{
   long  size;
   void  (*abort)(void);
   void *(*calloc)(unsigned long num_elements, unsigned long size);
   void  (*exit)(int status);
   void  (*free)(void *ptr);
   char *(*getenv)(const char *name);
   void *(*malloc)(unsigned long size);
   void *(*realloc)(void *ptr, unsigned long size);
   int   (*system)(const char *s);
   int   (*putenv)(const char *val);
   int   (*open)(const char *file, int mode, ...);
   int   (*access)(const char *filename, int flags);
   int   (*close)(int fd);
   int   (*lseek)(int fd, int offset, int whence);
   int   (*read)(int fd, void *buffer, unsigned long count);
   int   (*unlink)(const char *file);
   int   (*write)(int fd, const void *buffer, unsigned long count);
   int   (*isatty)(int fd);
   int   (*remove)(const char *file);
   int   (*rename)(const char *oldname, const char *newname);
   unsigned int (*time)(unsigned int *t);
   void  (*setfileattr)(const char *filename, unsigned attrib);
   unsigned long (*getcurrentdate)(void);
} LIBC_DATA;



/* setfileattr:
 *  Export function for modifying DOS file attributes.
 */
static void setfileattr(const char *filename, unsigned attrib)
{
   _dos_setfileattr(filename, attrib);
}



/* getcurrentdate:
 *  Export function for checking the current date. Returns the number of
 *  days since 1/1/1980.
 */
static unsigned long getcurrentdate(void)
{
   unsigned long t = time(NULL);

   /* this calculation _might_ be right, but somehow I doubt it :=) */

   t /= (24*60*60);     /* convert from seconds to days */
   t -= (365*10);       /* convert from 1970 origin to 1980 */
   t -= 2;              /* adjust for leap years in 1972 and 1976 */

   return t;
}



/* _fill_vbeaf_libc_exports:
 *  Provides libc function exports for the FAFEXT_LIBC extension.
 */
void _fill_vbeaf_libc_exports(void *ptr)
{
   LIBC_DATA *lc = (LIBC_DATA *)ptr;

   #define FILL_LIBC(field, func)                           \
   {                                                        \
      if ((long)offsetof(LIBC_DATA, field) < lc->size)      \
	 lc->field  = (void *)func;                         \
   }

   FILL_LIBC(abort, abort);
   FILL_LIBC(calloc, calloc);
   FILL_LIBC(exit, exit);
   FILL_LIBC(free, free);
   FILL_LIBC(getenv, getenv);
   FILL_LIBC(malloc, malloc);
   FILL_LIBC(realloc, realloc);
   FILL_LIBC(system, system);
   FILL_LIBC(putenv, putenv);
   FILL_LIBC(open, open);
   FILL_LIBC(access, access);
   FILL_LIBC(close, close);
   FILL_LIBC(lseek, lseek);
   FILL_LIBC(read, read);
   FILL_LIBC(unlink, unlink);
   FILL_LIBC(write, write);
   FILL_LIBC(isatty, isatty);
   FILL_LIBC(remove, remove);
   FILL_LIBC(rename, rename);
   FILL_LIBC(time, time);
   FILL_LIBC(setfileattr, setfileattr);
   FILL_LIBC(getcurrentdate, getcurrentdate);
}



/* This pmode export structure basically provides the SciTech pmode
 * library API, which is required by their Nucleus drivers. Why oh why
 * is there so !"$%^ much of it? They don't need nearly so many callbacks
 * just to write a simple video driver, and I resent being made to include
 * all this code just to use their drivers...
 */

typedef union
{
   struct {
      unsigned long eax, ebx, ecx, edx, esi, edi, cflag;
   } e;
   struct {
      unsigned short ax, ax_hi;
      unsigned short bx, bx_hi;
      unsigned short cx, cx_hi;
      unsigned short dx, dx_hi;
      unsigned short si, si_hi;
      unsigned short di, di_hi;
      unsigned short cflag, cflag_hi;
   } x;
   struct {
      unsigned char al, ah; unsigned short ax_hi;
      unsigned char bl, bh; unsigned short bx_hi;
      unsigned char cl, ch; unsigned short cx_hi;
      unsigned char dl, dh; unsigned short dx_hi;
   } h;
} SCITECH_REGS;



typedef struct
{
   unsigned short es, cs, ss, ds, fs, gs;
} SCITECH_SREGS;



/* FAFEXT_PMODE extension structure */
typedef struct PMODE_DATA
{
   long  size;
   int   (*getModeType)(void);
   void *(*getBIOSPointer)(void);
   void *(*getA0000Pointer)(void);
   void *(*mapPhysicalAddr)(unsigned long base, unsigned long limit);
   void *(*mallocShared)(long size);
   int   (*mapShared)(void *ptr);
   void  (*freeShared)(void *ptr);
   void *(*mapToProcess)(void *linear, unsigned long limit);
   void  (*loadDS)(void);
   void  (*saveDS)(void);
   void *(*mapRealPointer)(unsigned int r_seg, unsigned int r_off);
   void *(*allocRealSeg)(unsigned int size, unsigned int *r_seg, unsigned int *r_off);
   void  (*freeRealSeg)(void *mem);
   void *(*allocLockedMem)(unsigned int size, unsigned long *physAddr);
   void  (*freeLockedMem)(void *p);
   void  (*callRealMode)(unsigned int seg, unsigned int off, SCITECH_REGS *regs, SCITECH_SREGS *sregs);
   int   (*int86)(int intno, SCITECH_REGS *in, SCITECH_REGS *out);
   int   (*int86x)(int intno, SCITECH_REGS *in, SCITECH_REGS *out, SCITECH_SREGS *sregs);
   void  (*DPMI_int86)(int intno, __dpmi_regs *regs);
   void  (*segread)(SCITECH_SREGS *sregs);
   int   (*int386)(int intno, SCITECH_REGS *in, SCITECH_REGS *out);
   int   (*int386x)(int intno, SCITECH_REGS *in, SCITECH_REGS *out, SCITECH_SREGS *sregs);
   void  (*availableMemory)(unsigned long *physical, unsigned long *total);
   void *(*getVESABuf)(unsigned int *len, unsigned int *rseg, unsigned int *roff);
   long  (*getOSType)(void);
   void  (*fatalError)(const char *msg);
   void  (*setBankA)(int bank);
   void  (*setBankAB)(int bank);
   const char *(*getCurrentPath)(void);
   const char *(*getVBEAFPath)(void);
   const char *(*getNucleusPath)(void);
   const char *(*getNucleusConfigPath)(void);
   const char *(*getUniqueID)(void);
   const char *(*getMachineName)(void);
   int   (*VF_available)(void);
   void *(*VF_init)(unsigned long baseAddr, int bankSize, int codeLen, void *bankFunc);
   void  (*VF_exit)(void);
   int   (*kbhit)(void);
   int   (*getch)(void);
   int   (*openConsole)(void);
   int   (*getConsoleStateSize)(void);
   void  (*saveConsoleState)(void *stateBuf, int console_id);
   void  (*restoreConsoleState)(const void *stateBuf, int console_id);
   void  (*closeConsole)(int console_id);
   void  (*setOSCursorLocation)(int x, int y);
   void  (*setOSScreenWidth)(int width, int height);
   int   (*enableWriteCombine)(unsigned long base, unsigned long length);
   void  (*backslash)(char *filename);
} PMODE_DATA;



/* get_mode_type:
 *  Return that we are running in 386 mode.
 */
static int get_mode_type(void)
{
   return 2;      /* 0=real mode, 1=16 bit pmode, 2=32 bit pmode */
}



/* get_bios_pointer:
 *  Returns a pointer to the BIOS data area at segment 0x400.
 */
static void *get_bios_pointer(void)
{
   if (!(_crt0_startup_flags & _CRT0_FLAG_NEARPTR))
      if (__djgpp_nearptr_enable() == 0)
	 return NULL;

   return (void *)(__djgpp_conventional_base + 0x400);
}



/* get_a0000_pointer:
 *  Returns a linear pointer to the VGA frame buffer memory.
 */
static void *get_a0000_pointer(void)
{
   if (!(_crt0_startup_flags & _CRT0_FLAG_NEARPTR))
      if (__djgpp_nearptr_enable() == 0)
	 return NULL;

   return (void *)(__djgpp_conventional_base + 0xA0000);
}



/* map_physical_addr:
 *  Maps physical memory into the current DS segment.
 */
static void *map_physical_addr(unsigned long base, unsigned long limit)
{
   unsigned long linear;

   if (!(_crt0_startup_flags & _CRT0_FLAG_NEARPTR))
      if (__djgpp_nearptr_enable() == 0)
	 return NULL;

   if (_create_linear_mapping(&linear, base, limit) != 0)
      return NULL;

   return (void *)(__djgpp_conventional_base + linear);
}



/* malloc_shared:
 *  Allocates a memory block in the global shared region.
 */
static void *malloc_shared(long size)
{
   return _AL_MALLOC(size);
}



/* map_shared:
 *  Maps a shared memory block into the current process.
 */
static int map_shared(void *ptr)
{
   return 0;
}



/* free_shared:
 *  Frees the allocated shared memory block.
 */
static void free_shared(void *ptr)
{
   _AL_FREE(ptr);
}



/* map_to_process:
 *  Attaches a previously allocated linear mapping to a new process.
 */
static void *map_to_process(void *linear, unsigned long limit)
{
   return linear;
}



/* for the save_ds() / load_ds() functions */
static unsigned short saved_ds = 0;



/* save_ds:
 *  Saves the current data segment selector into a code segment variable.
 */
static void save_ds(void)
{
   saved_ds = _default_ds();
}



/* load_ds:
 *  Restores a data segment selector previously stored by save_ds().
 */
static void load_ds(void)
{
   #ifdef ALLEGRO_GCC

      /* use gcc-style inline asm */
      asm (
	 "  movw %%cs:_saved_ds, %%ax ; "
	 "  movw %%ax, %%ds "
      :
      :
      : "%eax"
      );

   #elif defined ALLEGRO_WATCOM

      /* use Watcom-style inline asm */
      {
	 int _ds(void);

	 #pragma aux _ds =                \
	    " mov ax, cs:saved_ds "       \
	    " mov ds, ax "                \
					  \
	 modify [eax];

	 _ds();
      }

   #endif
}



/* map_real_pointer:
 *  Maps a real mode pointer into our address space.
 */
static void *map_real_pointer(unsigned int r_seg, unsigned int r_off)
{
   if (!(_crt0_startup_flags & _CRT0_FLAG_NEARPTR))
      if (__djgpp_nearptr_enable() == 0)
	 return NULL;

   return (void *)(__djgpp_conventional_base + (r_seg<<4) + r_off);
}



/* cache so we know how to free real mode memory blocks */
static struct {
   void *ptr;
   int sel;
} rm_blocks[] =
{
   { NULL, 0 },   { NULL, 0 },   { NULL, 0 },   { NULL, 0 },
   { NULL, 0 },   { NULL, 0 },   { NULL, 0 },   { NULL, 0 },
   { NULL, 0 },   { NULL, 0 },   { NULL, 0 },   { NULL, 0 },
   { NULL, 0 },   { NULL, 0 },   { NULL, 0 },   { NULL, 0 }
};



/* alloc_real_seg:
 *  Allocates a block of conventional memory.
 */
static void *alloc_real_seg(unsigned int size, unsigned int *r_seg, unsigned int *r_off)
{
   int seg, sel, i;
   void *ptr;

   seg = __dpmi_allocate_dos_memory((size+15)>>4, &sel);

   if (seg < 0)
      return NULL;

   *r_seg = seg;
   *r_off = 0;

   ptr = map_real_pointer(seg, 0);

   for (i=0; i<(int)(sizeof(rm_blocks)/sizeof(rm_blocks[0])); i++) {
      if (!rm_blocks[i].ptr) {
	 rm_blocks[i].ptr = ptr;
	 rm_blocks[i].sel = sel;
	 break;
      }
   }

   return ptr;
}



/* free_real_seg:
 *  Frees a block of conventional memory.
 */
static void free_real_seg(void *mem)
{
   int i;

   for (i=0; i<(int)(sizeof(rm_blocks)/sizeof(rm_blocks[0])); i++) {
      if (rm_blocks[i].ptr == mem) {
	 __dpmi_free_dos_memory(rm_blocks[i].sel);
	 rm_blocks[i].ptr = NULL;
	 rm_blocks[i].sel = 0;
	 break;
      }
   }
}



/* alloc_locked_mem:
 *  Allocates a block of locked memory.
 */
static void *alloc_locked_mem(unsigned int size, unsigned long *physAddr)
{
   return NULL;
}



/* free_locked_mem:
 *  Frees a block of locked memory.
 */
static void free_locked_mem(void *p)
{
}



/* call_real_mode:
 *  Calls a real mode asm procedure.
 */
static void call_real_mode(unsigned int seg, unsigned int off, SCITECH_REGS *regs, SCITECH_SREGS *sregs)
{
   __dpmi_regs r;

   memset(&r, 0, sizeof(r));

   r.d.eax = regs->e.eax;
   r.d.ebx = regs->e.ebx;
   r.d.ecx = regs->e.ecx;
   r.d.edx = regs->e.edx;
   r.d.esi = regs->e.esi;
   r.d.edi = regs->e.edi;

   r.x.ds = sregs->ds;
   r.x.es = sregs->es;

   r.x.cs = seg;
   r.x.ip = off;

   __dpmi_simulate_real_mode_procedure_retf(&r);

   regs->e.eax = r.d.eax;
   regs->e.ebx = r.d.ebx;
   regs->e.ecx = r.d.ecx;
   regs->e.edx = r.d.edx;
   regs->e.esi = r.d.esi;
   regs->e.edi = r.d.edi;

   sregs->cs = r.x.cs;
   sregs->ds = r.x.ds;
   sregs->es = r.x.es;
   sregs->ss = r.x.ss;

   regs->x.cflag = (r.x.flags & 1);
}



/* my_int86:
 *  Generates a real mode interrupt.
 */
static int my_int86(int intno, SCITECH_REGS *in, SCITECH_REGS *out)
{
   __dpmi_regs r;

   memset(&r, 0, sizeof(r));

   r.d.eax = in->e.eax;
   r.d.ebx = in->e.ebx;
   r.d.ecx = in->e.ecx;
   r.d.edx = in->e.edx;
   r.d.esi = in->e.esi;
   r.d.edi = in->e.edi;

   __dpmi_simulate_real_mode_interrupt(intno, &r);

   out->e.eax = r.d.eax;
   out->e.ebx = r.d.ebx;
   out->e.ecx = r.d.ecx;
   out->e.edx = r.d.edx;
   out->e.esi = r.d.esi;
   out->e.edi = r.d.edi;

   out->x.cflag = (r.x.flags & 1);

   return out->x.ax;
}



/* my_int86x:
 *  Generates a real mode interrupt.
 */
static int my_int86x(int intno, SCITECH_REGS *in, SCITECH_REGS *out, SCITECH_SREGS *sregs)
{
   __dpmi_regs r;

   memset(&r, 0, sizeof(r));

   r.d.eax = in->e.eax;
   r.d.ebx = in->e.ebx;
   r.d.ecx = in->e.ecx;
   r.d.edx = in->e.edx;
   r.d.esi = in->e.esi;
   r.d.edi = in->e.edi;

   r.x.ds = sregs->ds;
   r.x.es = sregs->es;

   __dpmi_simulate_real_mode_interrupt(intno, &r);

   out->e.eax = r.d.eax;
   out->e.ebx = r.d.ebx;
   out->e.ecx = r.d.ecx;
   out->e.edx = r.d.edx;
   out->e.esi = r.d.esi;
   out->e.edi = r.d.edi;

   sregs->cs = r.x.cs;
   sregs->ds = r.x.ds;
   sregs->es = r.x.es;
   sregs->ss = r.x.ss;

   out->x.cflag = (r.x.flags & 1);

   return out->x.ax;
}



/* dpmi_int86:
 *  Generates a real mode interrupt.
 */
static void dpmi_int86(int intno, __dpmi_regs *regs)
{
   __dpmi_simulate_real_mode_interrupt(intno, regs);
}



/* my_segread:
 *  Reads the current selector values.
 */
static void my_segread(SCITECH_SREGS *sregs)
{
   #ifdef ALLEGRO_GCC

      /* use gcc-style inline asm */
      asm (
	 "  movw %%cs, %w0 ; "
	 "  movw %%ds, %w1 ; "
	 "  movw %%es, %w2 ; "
	 "  movw %%fs, %w3 ; "
	 "  movw %%gs, %w4 ; "
	 "  movw %%ss, %w5 ; "

      : "=m" (sregs->cs),
	"=m" (sregs->ds),
	"=m" (sregs->es),
	"=m" (sregs->fs),
	"=m" (sregs->gs),
	"=m" (sregs->ss)
      );

   #elif defined WATCOM

      segread((struct SREGS *)sregs);

   #endif
}



#ifdef ALLEGRO_GCC      /* gcc version of my_int386x() */



/* code fragment for issuing interrupt calls */
#define INT(n)             \
{                          \
   0xCD,    /* int */      \
   n,                      \
   0xC3,    /* ret */      \
}


#define INTROW(n)                                           \
   INT(n+0x00), INT(n+0x01), INT(n+0x02), INT(n+0x03),      \
   INT(n+0x04), INT(n+0x05), INT(n+0x06), INT(n+0x07),      \
   INT(n+0x08), INT(n+0x09), INT(n+0x0A), INT(n+0x0B),      \
   INT(n+0x0C), INT(n+0x0D), INT(n+0x0E), INT(n+0x0F)


static unsigned char asm_int_code[256][3] = 
{ 
   INTROW(0x00), INTROW(0x10), INTROW(0x20), INTROW(0x30),
   INTROW(0x40), INTROW(0x50), INTROW(0x60), INTROW(0x70),
   INTROW(0x80), INTROW(0x90), INTROW(0xA0), INTROW(0xB0),
   INTROW(0xC0), INTROW(0xD0), INTROW(0xE0), INTROW(0xF0)
};


#undef INT
#undef INTROW



/* temporary global for storing the data selector
 * FIXME: this is supposed to be static but that doesn't work with the
 * following asm code with gcc 4
 */
int __al_vbeafex_int_ds;



/* my_int386x:
 *  Generates a protected mode interrupt (gcc version).
 */
static int my_int386x(int intno, SCITECH_REGS *in, SCITECH_REGS *out, SCITECH_SREGS *sregs)
{
   asm (
      "  pushal ; "                       /* push lots of stuff */
      "  pushw %%es ; "
      "  pushw %%fs ; "
      "  pushw %%gs ; "
      "  pushl %%esi ; "
      "  pushl %%edx ; "
      "  pushw %%ds ; "
      "  pushl %%ecx ; "

      "  movw (%%esi), %%es ; "           /* load selectors */
      "  movw 6(%%esi), %%ax ; "
      "  movw %%ax, ___al_vbeafex_int_ds ; "
      "  movw 8(%%esi), %%fs ; "
      "  movw 10(%%esi), %%gs ; "

      "  movl (%%edi), %%eax ; "          /* load registers */
      "  movl 4(%%edi), %%ebx ; "
      "  movl 8(%%edi), %%ecx ; "
      "  movl 12(%%edi), %%edx ; "
      "  movl 16(%%edi), %%esi ; "
      "  movl 20(%%edi), %%edi ; "

      "  movw ___al_vbeafex_int_ds, %%ds ; " /* load %ds selector */

      "  clc ; "                          /* generate the interrupt */
      "  popl %%ebp ; "
      "  call *%%ebp ; "

      "  movw %%ds, %%ss:___al_vbeafex_int_ds ; " /* store returned %ds value */
      "  popw %%ds ; "                    /* restore original %ds */

      "  movl %%edi, %%ebp ; "            /* store %edi */
      "  popl %%edi ; "                   /* pop output pointer into %edi */

      "  movl %%eax, (%%edi) ; "          /* store output registers */
      "  movl %%ebx, 4(%%edi) ; "
      "  movl %%ecx, 8(%%edi) ; "
      "  movl %%edx, 12(%%edi) ; "
      "  movl %%esi, 16(%%edi) ; "
      "  movl %%ebp, 20(%%edi) ; "

      "  pushfl ; "                       /* store output carry flag */
      "  popl %%eax ; "
      "  andl $1, %%eax ; "
      "  movl %%eax, 24(%%edi) ; "

      "  popl %%esi ; "                   /* pop sregs pointer into %esi */

      "  movw %%es, (%%esi) ; "           /* store output selectors */
      "  movw ___al_vbeafex_int_ds, %%ax ; "
      "  movw %%ax, 6(%%esi) ; "
      "  movw %%fs, 8(%%esi) ; "
      "  movw %%gs, 10(%%esi) ; "

      "  popw %%gs ; "                    /* pop remaining values */
      "  popw %%fs ; "
      "  popw %%es ; "
      "  popal ; "

   :                                      /* no outputs */

   : "S" (sregs),                         /* sregs in %esi */
     "D" (in),                            /* in pointer in %edi */
     "d" (out),                           /* out pointer in %edx */
     "c" (asm_int_code[intno])            /* function pointer in %ecx */
   );

   return out->e.eax;
}



#elif defined ALLEGRO_WATCOM



/* my_int386x:
 *  Generates a protected mode interrupt (Watcom version).
 */
static int my_int386x(int intno, SCITECH_REGS *in, SCITECH_REGS *out, SCITECH_SREGS *sregs)
{
   return int386x(intno, (union REGS *)in, (union REGS *)out, (struct SREGS *)sregs);
}



#endif      /* gcc vs. Watcom */



/* my_int386:
 *  Generates a protected mode interrupt.
 */
static int my_int386(int intno, SCITECH_REGS *in, SCITECH_REGS *out)
{
   SCITECH_SREGS sregs;

   my_segread(&sregs);

   return my_int386x(intno, in, out, &sregs);
}



/* available_memory:
 *  Returns the amount of available free memory.
 */
static void available_memory(unsigned long *physical, unsigned long *total)
{
   __dpmi_free_mem_info info;

   __dpmi_get_free_memory_information(&info);

   *physical = info.total_number_of_free_pages * 4096;
   *total = info.largest_available_free_block_in_bytes;

   if (*total < *physical)
      *physical = *total;
}



/* conventional memory buffer for communicating with VESA */
static void *vesa_ptr = NULL;



/* free_vesa_buf:
 *  Cleanup routine.
 */
static void free_vesa_buf(void)
{
   if (vesa_ptr) {
      free_real_seg(vesa_ptr);
      vesa_ptr = NULL;
   }
}



/* get_vesa_buf:
 *  Returns the address of a global VESA real mode transfer buffer.
 */
static void *get_vesa_buf(unsigned int *len, unsigned int *rseg, unsigned int *roff)
{
   static unsigned int seg, off;

   if (!vesa_ptr) {
      vesa_ptr = alloc_real_seg(2048, &seg, &off);

      if (!vesa_ptr)
	 return NULL;

      atexit(free_vesa_buf);
   }

   *len = 2048;
   *rseg = seg;
   *roff = off;

   return vesa_ptr;
}



/* get_os_type:
 *  Returns the OS type flag.
 */
static long get_os_type(void)
{
   return 1;      /* _OS_DOS */
}



/* fatal_error:
 *  Handles a fatal error condition.
 */
static void fatal_error(const char *msg)
{
   fprintf(stderr, "%s\n", msg);
   fflush(stderr);
   exit(1);
}



/* set_banka:
 *  Sets a VBE bank using int 0x10.
 */
static void set_banka(int bank)
{
   __dpmi_regs r;

   r.x.ax = 0x4F05;
   r.x.bx = 0;
   r.x.dx = bank;

   __dpmi_int(0x10, &r);
}



/* set_bankab:
 *  Sets both VBE banks using int 0x10.
 */
static void set_bankab(int bank)
{
   __dpmi_regs r;

   r.x.ax = 0x4F05;
   r.x.bx = 0;
   r.x.dx = bank;

   __dpmi_int(0x10, &r);

   r.x.ax = 0x4F05;
   r.x.bx = 1;
   r.x.dx = bank;

   __dpmi_int(0x10, &r);
}



/* get_current_path:
 *  Returns the current working directory.
 */
static const char *get_current_path(void)
{
   static char *buffer = NULL;

   if (!buffer)
      buffer = _AL_MALLOC(BUFFER_SIZE);

   getcwd(buffer, BUFFER_SIZE-1);

   return buffer;
}



/* get_vbeaf_path:
 *  Returns the VBE/AF driver directory.
 */
static const char *get_vbeaf_path(void)
{
   return "c:\\";
}



/* get_nucleus_path:
 *  Returns the Nucleus driver directory.
 */
static const char *get_nucleus_path(void)
{
   static char *buffer = NULL;
   char *p;

   p = getenv("NUCLEUS_PATH");
   if (p)
      return p;

   p = getenv("WINBOOTDIR");
   if (p) {
      if (!buffer)
	 buffer = _AL_MALLOC(BUFFER_SIZE);

      _al_sane_strncpy(buffer , p, BUFFER_SIZE);
      strncat(buffer, "\\nucleus", BUFFER_SIZE-1);
      return buffer;
   }

   return "c:\\nucleus";
}



/* get_nucleus_config_path:
 *  Returns the Nucleus config directory.
 */
static const char *get_nucleus_config_path(void)
{
   static char *buffer = NULL;

   if (!buffer)
      buffer = _AL_MALLOC(BUFFER_SIZE);

   _al_sane_strncpy(buffer, get_nucleus_path(), BUFFER_SIZE);
   put_backslash(buffer);
   strncat(buffer, "config", BUFFER_SIZE-1);

   return buffer;
}



/* get_unique_id:
 *  Returns a network unique machine identifier as a string.
 */
static const char *get_unique_id(void)
{
   return "DOS";
}



/* get_machine_name:
 *  Returns the network machine name as a string.
 */
static const char *get_machine_name(void)
{
   static char *buffer = NULL;

   if (!buffer)
      buffer = _AL_MALLOC(BUFFER_SIZE);

   #ifdef ALLEGRO_DJGPP
      gethostname(buffer, BUFFER_SIZE-1);
   #else
      _al_sane_strncpy(buffer, "pc", BUFFER_SIZE);
   #endif

   return buffer;
}



/* vf_available:
 *  Checks whether the virtual framebuffer mode is avaliable (no, it isn't).
 */
static int vf_available(void)
{
   return 0;
}



/* vf_init:
 *  Initialises the virtual framebuffer mode.
 */
static void *vf_init(unsigned long baseAddr, int bankSize, int codeLen, void *bankFunc)
{
   return NULL;
}



/* vf_exit:
 *  Shuts down the virtual framebuffer mode.
 */
static void vf_exit(void)
{
}



/* stored console state structure */
typedef struct 
{
   int mode;
   int tall;
} CONSOLE_STATE;



/* open_console:
 *  Prepares the system for console output.
 */
static int open_console(void)
{
   return 0;
}



/* get_console_state_size:
 *  Returns the size of a console state buffer.
 */
static int get_console_state_size(void)
{
   return sizeof(CONSOLE_STATE);
}



/* save_console_state:
 *  Stores the current console status.
 */
static void save_console_state(void *stateBuf, int console_id)
{
   CONSOLE_STATE *state = stateBuf;
   __dpmi_regs r;

   r.x.ax = 0x0F00;
   __dpmi_int(0x10, &r);

   state->mode = r.h.al & 0x7F;

   if (state->mode == 0x3) {
      r.x.ax = 0x1130;
      r.x.bx = 0;
      r.x.dx = 0;
      __dpmi_int(0x10, &r);

      state->tall = ((r.h.dl == 42) || (r.h.dl == 49));
   }
   else
      state->tall = FALSE;
}



/* restore_console_state:
 *  Restores a previously saved console status.
 */
static void restore_console_state(const void *stateBuf, int console_id)
{
   const CONSOLE_STATE *state = stateBuf;
   __dpmi_regs r;

   if (state->tall) {
      r.x.ax = 0x1112;
      r.x.bx = 0;
      __dpmi_int(0x10, &r);
   }
}



/* close_console:
 *  Shuts down the console mode.
 */
static void close_console(int console_id)
{
}



/* set_os_cursor_location:
 *  Positions the text mode cursor.
 */
static void set_os_cursor_location(int x, int y)
{
   _farsetsel(_dos_ds);
   _farnspokeb(0x450, x);
   _farnspokeb(0x451, y);
}



/* set_os_screen_width:
 *  Sets the console width.
 */
static void set_os_screen_width(int width, int height)
{
   _farnspokeb(0x44A, width);
   _farnspokeb(0x484, height-1);
}



/* enable_write_combine:
 *  Enables the Intel PPro/PII write combining.
 */
static int enable_write_combine(unsigned long base, unsigned long length)
{
   return 0;
}



/* _fill_vbeaf_pmode_exports:
 *  Provides pmode function exports for the FAFEXT_PMODE extension.
 */
void _fill_vbeaf_pmode_exports(void *ptr)
{
   PMODE_DATA *pm = (PMODE_DATA *)ptr;

   #define FILL_PMODE(field, func)                          \
   {                                                        \
      if ((long)offsetof(PMODE_DATA, field) < pm->size)     \
	 pm->field = func;                                  \
   }

   FILL_PMODE(getModeType, get_mode_type);
   FILL_PMODE(getBIOSPointer, get_bios_pointer);
   FILL_PMODE(getA0000Pointer, get_a0000_pointer);
   FILL_PMODE(mapPhysicalAddr, map_physical_addr);
   FILL_PMODE(mallocShared, malloc_shared);
   FILL_PMODE(mapShared, map_shared);
   FILL_PMODE(freeShared, free_shared);
   FILL_PMODE(mapToProcess, map_to_process);
   FILL_PMODE(loadDS, load_ds);
   FILL_PMODE(saveDS, save_ds);
   FILL_PMODE(mapRealPointer, map_real_pointer);
   FILL_PMODE(allocRealSeg, alloc_real_seg);
   FILL_PMODE(freeRealSeg, free_real_seg);
   FILL_PMODE(allocLockedMem, alloc_locked_mem);
   FILL_PMODE(freeLockedMem, free_locked_mem);
   FILL_PMODE(callRealMode, call_real_mode);
   FILL_PMODE(int86, my_int86);
   FILL_PMODE(int86x, my_int86x);
   FILL_PMODE(DPMI_int86, dpmi_int86);
   FILL_PMODE(segread, my_segread);
   FILL_PMODE(int386, my_int386);
   FILL_PMODE(int386x, my_int386x);
   FILL_PMODE(availableMemory, available_memory);
   FILL_PMODE(getVESABuf, get_vesa_buf);
   FILL_PMODE(getOSType, get_os_type);
   FILL_PMODE(fatalError, fatal_error);
   FILL_PMODE(setBankA, set_banka);
   FILL_PMODE(setBankAB, set_bankab);
   FILL_PMODE(getCurrentPath, get_current_path);
   FILL_PMODE(getVBEAFPath, get_vbeaf_path);
   FILL_PMODE(getNucleusPath, get_nucleus_path);
   FILL_PMODE(getNucleusConfigPath, get_nucleus_config_path);
   FILL_PMODE(getUniqueID, get_unique_id);
   FILL_PMODE(getMachineName, get_machine_name);
   FILL_PMODE(VF_available, vf_available);
   FILL_PMODE(VF_init, vf_init);
   FILL_PMODE(VF_exit, vf_exit);
   FILL_PMODE(kbhit, kbhit);
   FILL_PMODE(getch, getch);
   FILL_PMODE(openConsole, open_console);
   FILL_PMODE(getConsoleStateSize, get_console_state_size);
   FILL_PMODE(saveConsoleState, save_console_state);
   FILL_PMODE(restoreConsoleState, restore_console_state);
   FILL_PMODE(closeConsole, close_console);
   FILL_PMODE(setOSCursorLocation, set_os_cursor_location);
   FILL_PMODE(setOSScreenWidth, set_os_screen_width);
   FILL_PMODE(enableWriteCombine, enable_write_combine);
   FILL_PMODE(backslash, put_backslash);
}


