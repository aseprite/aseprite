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
 *      Main system driver for the DOS library.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef SCAN_DEPEND
   #include <io.h>
   #include <dos.h>
   #include <stdio.h>
   #include <conio.h>
   #include <signal.h>
   #include <string.h>
   #include <setjmp.h>
#endif

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintdos.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif


#ifndef SCAN_DEPEND

   #ifdef ALLEGRO_DJGPP
      /* these headers only apply to djgpp */
      #include <crt0.h>
      #include <sys/exceptn.h>
      #include <sys/nearptr.h>
   #endif


   #ifdef ALLEGRO_WATCOM
      /* horrible hackery here, so the rest of the file will work unchanged */
      #include <process.h>

      #define _write                   write
      #define _get_dos_version(n)      ((_osmajor << 8) | _osminor)
      #define ScreenRows()             -1
      #define _set_screen_lines(n)
   #endif

#endif



/* some DOS-specific globals */
int i_love_bill = FALSE;

static int a_rez = 3;
static int a_lines = -1;

static int console_virgin = TRUE;


/* previous signal handlers */
static void *old_sig_abrt = NULL;
static void *old_sig_fpe  = NULL;
static void *old_sig_ill  = NULL;
static void *old_sig_segv = NULL;
static void *old_sig_term = NULL;
static void *old_sig_int  = NULL;

#ifdef SIGKILL
   static void *old_sig_kill = NULL;
#endif

#ifdef SIGQUIT
   static void *old_sig_quit = NULL;
#endif

#ifdef SIGTRAP
   static void *old_sig_trap = NULL;
#endif


/* system driver functions */
static int sys_dos_init(void);
static void sys_dos_exit(void);
static void sys_dos_get_executable_name(char *output, int size);
static void sys_dos_save_console_state(void);
static void sys_dos_restore_console_state(void);
static void sys_dos_read_palette(void);
static void sys_dos_set_palette(AL_CONST struct RGB *p, int from, int to, int vsync);
static void sys_dos_get_gfx_safe_mode(int *driver, struct GFX_MODE *mode);
static void sys_dos_yield_timeslice(void);

#ifdef ALLEGRO_DJGPP
   static void sys_dos_assert(AL_CONST char *msg);
#else
   #define sys_dos_assert  NULL
#endif


/* the main system driver for running under dos */
SYSTEM_DRIVER system_dos =
{
   SYSTEM_DOS,   /* id */
   empty_string, /* name */
   empty_string, /* desc */
   "DOS",        /* ascii_name */
   sys_dos_init,
   sys_dos_exit,
   sys_dos_get_executable_name,
   NULL, /* find_resource */
   NULL, /* set_window_title */
   NULL, /* set_close_button_callback */
   NULL, /* message */
   sys_dos_assert,
   sys_dos_save_console_state,
   sys_dos_restore_console_state,
   NULL, /* create_bitmap */
   NULL, /* created_bitmap */
   NULL, /* create_sub_bitmap */
   NULL, /* created_sub_bitmap */
   NULL, /* destroy_bitmap */
   sys_dos_read_palette,
   sys_dos_set_palette,
   NULL, /* get_vtable */
   NULL, /* set_display_switch_mode */
   NULL, /* display_switch_lock */
   NULL, /* desktop_color_depth */
   NULL, /* get_desktop_resolution */
   sys_dos_get_gfx_safe_mode,
   sys_dos_yield_timeslice,
   NULL, /* create_mutex */
   NULL, /* destroy_mutex */
   NULL, /* lock_mutex */
   NULL, /* unlock_mutex */
   NULL, /* gfx_drivers */
   NULL, /* digi_drivers */
   NULL, /* midi_drivers */
   NULL, /* keyboard_drivers */
   NULL, /* mouse_drivers */
   NULL, /* joystick_drivers */
   NULL  /* timer_drivers */
};



/* list of available system drivers */
_DRIVER_INFO _system_driver_list[] =
{
   {  SYSTEM_DOS,    &system_dos,   TRUE  },
   {  SYSTEM_NONE,   &system_none,  FALSE },
   {  0,             NULL,          0     }
};



/* signal_handler:
 *  Used to trap various signals, to make sure things get shut down cleanly.
 */
static void signal_handler(int num)
{
   static char msg[] = "Shutting down Allegro\r\n";

   allegro_exit();

   _write(STDERR_FILENO, msg, sizeof(msg)-1);

   raise(num);
}



/* detect_os:
 *  Operating system autodetection routine.
 */
static void detect_os(void)
{
   __dpmi_regs r;
   char buf[16];
   char *p;
   int i;

   os_type = OSTYPE_UNKNOWN;

   /* check for Windows 3.x or 9x */
   r.x.ax = 0x1600; 
   __dpmi_int(0x2F, &r);

   if ((r.h.al != 0) && (r.h.al != 1) && (r.h.al != 0x80) && (r.h.al != 0xFF)) {
      os_version = r.h.al;
      os_revision = r.h.ah;
      os_multitasking = TRUE;

      if (os_version == 4) {
         if (os_revision == 90)
            os_type = OSTYPE_WINME;
         else if (os_revision == 10)
            os_type = OSTYPE_WIN98;
	 else
            os_type = OSTYPE_WIN95;
      }
      else
	 os_type = OSTYPE_WIN3;

      i_love_bill = TRUE;
      return;
   }

   /* check for Windows NT */
   p = getenv("OS");

   if (((p) && (stricmp(p, "Windows_NT") == 0)) || (_get_dos_version(1) == 0x0532)) {
      os_type = OSTYPE_WINNT;
      os_multitasking = TRUE;
      i_love_bill = TRUE;
      return;
   }

   /* check for OS/2 */
   r.x.ax = 0x4010;
   __dpmi_int(0x2F, &r);

   if (r.x.ax != 0x4010) {
      if (r.x.ax == 0)
	 os_type = OSTYPE_WARP;
      else
	 os_type = OSTYPE_OS2;

      os_multitasking = TRUE;
      i_love_bill = TRUE;
      return;
   }

   /* check for Linux DOSEMU */
   _farsetsel(_dos_ds);

   for (i=0; i<8; i++) 
      buf[i] = _farnspeekb(0xFFFF5+i);

   buf[8] = 0;

   if (!strcmp(buf, "02/25/93")) {
      r.x.ax = 0;
      __dpmi_int(0xE6, &r);

      if (r.x.ax == 0xAA55) {
	 os_type = OSTYPE_DOSEMU;
	 os_multitasking = TRUE;
	 i_love_bill = TRUE;     /* (evil chortle) */
	 return;
      }
   }

   /* check if running under OpenDOS */
   r.x.ax = 0x4452;
   __dpmi_int(0x21, &r);

   if ((r.x.ax >= 0x1072) && !(r.x.flags & 1)) {
      os_type = OSTYPE_OPENDOS;

      /* now check for OpenDOS EMM386.EXE */
      r.x.ax = 0x12FF;
      r.x.bx = 0x0106;
      __dpmi_int(0x2F, &r);

      if ((r.x.ax == 0) && (r.x.bx == 0xEDC0)) {
	 i_love_bill = TRUE;
      }

      return;
   }

   /* check for that stupid win95 "stealth mode" setting */
   r.x.ax = 0x8102;
   r.x.bx = 0;
   r.x.dx = 0;
   __dpmi_int(0x4B, &r);

   if ((r.x.bx == 3) && !(r.x.flags & 1)) {
      os_type = OSTYPE_WIN95;
      os_multitasking = TRUE;
      i_love_bill = TRUE;
      return;
   }

   /* fetch DOS version if pure DOS is likely to be the running OS */
   if (os_type == OSTYPE_UNKNOWN) {
      r.x.ax = 0x3000;
      __dpmi_int(0x21, &r);
      os_version = r.h.al;
      os_revision = r.h.ah;
   }
}



/* sys_dos_init:
 *  Top level system driver wakeup call.
 */
static int sys_dos_init(void)
{
   #ifdef ALLEGRO_DJGPP
      /* make sure djgpp won't move our memory around */
      _crt0_startup_flags &= ~_CRT0_FLAG_UNIX_SBRK;
      _crt0_startup_flags |= _CRT0_FLAG_NONMOVE_SBRK;
   #endif

   /* initialise the irq wrapper functions */
   _dos_irq_init();

   /* check which OS we are running under */
   detect_os();

   /* detect CRTC register address */
   _vga_regs_init();

   /* install emergency-exit signal handlers */
   old_sig_abrt = signal(SIGABRT, signal_handler);
   old_sig_fpe  = signal(SIGFPE,  signal_handler);
   old_sig_ill  = signal(SIGILL,  signal_handler);
   old_sig_segv = signal(SIGSEGV, signal_handler);
   old_sig_term = signal(SIGTERM, signal_handler);
   old_sig_int  = signal(SIGINT,  signal_handler);

   #ifdef SIGKILL
      old_sig_kill = signal(SIGKILL, signal_handler);
   #endif

   #ifdef SIGQUIT
      old_sig_quit = signal(SIGQUIT, signal_handler);
   #endif

   #ifdef SIGTRAP
      old_sig_trap = signal(SIGTRAP, signal_handler);
   #endif

   return 0;
}



/* sys_dos_exit:
 *  The end of the world...
 */
static void sys_dos_exit(void)
{
   _dos_irq_exit();

   signal(SIGABRT, old_sig_abrt);
   signal(SIGFPE,  old_sig_fpe);
   signal(SIGILL,  old_sig_ill);
   signal(SIGSEGV, old_sig_segv);
   signal(SIGTERM, old_sig_term);
   signal(SIGINT,  old_sig_int);

   #ifdef SIGKILL
      signal(SIGKILL, old_sig_kill);
   #endif

   #ifdef SIGQUIT
      signal(SIGQUIT, old_sig_quit);
   #endif

   #ifdef SIGTRAP
      signal(SIGTRAP, old_sig_trap);
   #endif
}



/* sys_dos_get_executable_name:
 *  Return full path to the current executable.
 */
static void sys_dos_get_executable_name(char *output, int size)
{
   #ifdef ALLEGRO_DJGPP

      /* djgpp stores the program name in the __crt0_argv[] global */
      do_uconvert(__crt0_argv[0], U_ASCII, output, U_CURRENT, size);

   #elif defined ALLEGRO_WATCOM

      /* Watcom has a _cmdname() function to fetch the program name */
      char buf[1024];
      do_uconvert(_cmdname(buf), U_ASCII, output, U_CURRENT, size);

   #else
      #error unknown platform
   #endif
}



#ifdef ALLEGRO_DJGPP


/* sys_dos_assert:
 *  Handles an assert failure, generating a stack traceback. This routine
 *  is heavily based on dpmiexcp.c from the djgpp libc sources, by Charles
 *  Sandmann. It is reimplemented here because we don't want the register
 *  dump, just the stack traceback, and so we can strip off the top couple
 *  of stack entries (users don't need to see inside the Allegro assert
 *  mechanism). And because it is cool.
 */
static void sys_dos_assert(AL_CONST char *msg)
{
   extern unsigned int end __asm__ ("end");
   extern unsigned int _stklen;
   unsigned int *vbp, *vbp_new, *tos, veip;
   int max, c;
   jmp_buf j;

   setjmp(j);

   allegro_exit();

   fprintf(stderr, "%s\n\n", msg);
   fprintf(stderr, "Call frame traceback EIPs:\n");

   tos = (unsigned int *)__djgpp_selector_limit;
   vbp = (unsigned int *)j->__ebp;

   if (isatty(fileno(stderr))) {
      max = _farpeekb(_dos_ds, 0x484) + 1;
      if ((max < 10) || (max > 75))
	 max = 19;
      else
	 max -= 6;
   }
   else
      max = _stklen/8;

   c = 0;

   while (((unsigned int)vbp >= j->__esp) && (vbp >= &end) && (vbp < tos)) {
      vbp_new = (unsigned int *)vbp[0];
      if (!vbp_new)
	 break;
      veip = vbp[1];
      if (c++)
	 fprintf(stderr, "  0x%08x\n", veip);
      vbp = vbp_new;
      if (--max <= 0)
	 break;
   } 

   exit(1);
}


#endif



/* sys_dos_save_console_state:
 *  Called just before switching into a graphics mode, to remember what
 *  the text console was like.
 */
static void sys_dos_save_console_state(void)
{
   __dpmi_regs r;
   int c;

   if (!console_virgin)
      return;

   for (c=0; c<256; c++) {          /* store current color palette */
      outportb(0x3C7, c);
      _current_palette[c].r = inportb(0x3C9);
      _current_palette[c].g = inportb(0x3C9);
      _current_palette[c].b = inportb(0x3C9);
   }

   r.x.ax = 0x0F00;                 /* store current video mode */
   __dpmi_int(0x10, &r);
   a_rez = r.x.ax & 0xFF; 

   if (a_rez > 19)                  /* ignore non-standard modes */
      a_rez = 3;

   if (a_rez == 3)                  /* store current screen height */
      a_lines = ScreenRows();
   else
      a_lines = -1;

   console_virgin = FALSE;
}



/* sys_dos_restore_console_state:
 *  Puts us back into the original text mode.
 */
static void sys_dos_restore_console_state(void)
{
   __dpmi_regs r;

   r.x.ax = 0x0F00; 
   __dpmi_int(0x10, &r);

   if ((r.x.ax & 0xFF) != a_rez) {
      r.x.ax = a_rez;
      __dpmi_int(0x10, &r);
   }

   if (a_rez == 3) {
      if (ScreenRows() != a_lines)
	 _set_screen_lines(a_lines);
   }
}



/* sys_dos_read_palette:
 *  Reads the current palette from the video hardware.
 */
static void sys_dos_read_palette(void)
{
   if (console_virgin)
      sys_dos_save_console_state();
}



/* sys_dos_set_palette:
 *  Writes a palette to the video hardware.
 */
static void sys_dos_set_palette(AL_CONST struct RGB *p, int from, int to, int vsync)
{
   if (console_virgin)
      sys_dos_save_console_state();

   _vga_set_palette_range(p, from, to, vsync);
}



/* sys_dos_get_gfx_safe_mode:
 *  Defines the safe graphics mode for this system.
 */
static void sys_dos_get_gfx_safe_mode(int *driver, struct GFX_MODE *mode)
{
   *driver = GFX_VGA;
   mode->width = 320;
   mode->height = 200;
   mode->bpp = 8;
}



/* _set_vga_mode:
 *  Helper for the VGA and mode-X drivers to set a video mode.
 */
uintptr_t _set_vga_mode(int modenum)
{
   __dpmi_regs r;

   if (modenum >= 0x100) {
      /* set VESA mode */
      r.x.ax = 0x4F02;
      r.x.bx = modenum;

      __dpmi_int(0x10, &r);

      if (r.h.ah)
	 return 0;
   }
   else {
      /* set VGA mode */
      r.x.ax = modenum; 
      __dpmi_int(0x10, &r);
   }

   return 0xA0000;
}



/* _unset_vga_mode:
 *  Helper for the VGA and mode-X drivers to unset a video mode.
 */
void _unset_vga_mode(void)
{
   /* nothing to be done in DOS */
}



/* _save_vga_mode:
 *  Helper for VGA and mode-X drivers to save state.
 */
void _save_vga_mode(void)
{
   /* nothing to be done in DOS */
}



/* _restore_vga_mode:
 *  Helper to restore previously saved mode.
 */
void _restore_vga_mode(void)
{
   /* nothing to be done in DOS */
}



/* sys_dos_yield_timeslice:
 *  Yields the remaining timeslice portion to the system
 */
static void sys_dos_yield_timeslice(void)
{
   #ifdef ALLEGRO_DJGPP

      __dpmi_yield();

   #endif
}

