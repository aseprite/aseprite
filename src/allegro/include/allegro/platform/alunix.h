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
 *      Unix-specific header defines.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_UNIX
   #error bad include
#endif


/* magic to capture name of executable file */
extern int    __crt0_argc;
extern char **__crt0_argv;

#ifdef ALLEGRO_WITH_MAGIC_MAIN

   #ifndef ALLEGRO_NO_MAGIC_MAIN
      #define ALLEGRO_MAGIC_MAIN
      #define main _mangled_main
      #undef END_OF_MAIN
      #define END_OF_MAIN() void *_mangled_main_address = (void*) _mangled_main;
   #else
      #undef END_OF_MAIN
      #define END_OF_MAIN() void *_mangled_main_address;
   #endif

#endif



/**************************************/
/************ General Unix ************/
/**************************************/

#define TIMERDRV_UNIX_PTHREADS	AL_ID('P','T','H','R')
#define TIMERDRV_UNIX_SIGALRM    AL_ID('A','L','R','M')


#ifdef ALLEGRO_HAVE_LIBPTHREAD
AL_VAR(TIMER_DRIVER, timerdrv_unix_pthreads);
#else
AL_VAR(TIMER_DRIVER, timerdrv_unix_sigalrm);
#endif



/************************************/
/*********** Sound drivers **********/
/************************************/

#define DIGI_OSS              AL_ID('O','S','S','D')
#define MIDI_OSS              AL_ID('O','S','S','M')
#define DIGI_ESD              AL_ID('E','S','D','D')
#define DIGI_ARTS             AL_ID('A','R','T','S')
#define DIGI_SGIAL            AL_ID('S','G','I','A')
#define DIGI_ALSA             AL_ID('A','L','S','A')
#define MIDI_ALSA             AL_ID('A','M','I','D')
#define DIGI_JACK             AL_ID('J','A','C','K')


#ifdef ALLEGRO_WITH_OSSDIGI
AL_VAR(DIGI_DRIVER, digi_oss);
#define DIGI_DRIVER_OSS                                          \
      {  DIGI_OSS,        &digi_oss,            TRUE  },
#endif /* ALLEGRO_WITH_OSSDIGI */

#ifdef ALLEGRO_WITH_OSSMIDI
AL_VAR(MIDI_DRIVER, midi_oss);
#define MIDI_DRIVER_OSS                                          \
      {  MIDI_OSS,        &midi_oss,            TRUE  },
#endif /* ALLEGRO_WITH_OSSMIDI */

#ifndef ALLEGRO_WITH_MODULES

#ifdef ALLEGRO_WITH_ESDDIGI
AL_VAR(DIGI_DRIVER, digi_esd);
#define DIGI_DRIVER_ESD                                          \
      {  DIGI_ESD,        &digi_esd,            TRUE  },
#endif /* ALLEGRO_WITH_ESDDIGI */

#ifdef ALLEGRO_WITH_ARTSDIGI
AL_VAR(DIGI_DRIVER, digi_arts);
#define DIGI_DRIVER_ARTS                                         \
      {  DIGI_ARTS,       &digi_arts,            TRUE  },
#endif /* ALLEGRO_WITH_ARTSDIGI */

#ifdef ALLEGRO_WITH_SGIALDIGI
AL_VAR(DIGI_DRIVER, digi_sgial);
#define DIGI_DRIVER_SGIAL                                        \
      {  DIGI_SGIAL,      &digi_sgial,          TRUE  },
#endif /* ALLEGRO_WITH_SGIALDIGI */

#ifdef ALLEGRO_WITH_ALSADIGI
AL_VAR(DIGI_DRIVER, digi_alsa);
#define DIGI_DRIVER_ALSA                                         \
      {  DIGI_ALSA,       &digi_alsa,           TRUE  },
#endif /* ALLEGRO_WITH_ALSADIGI */


#ifdef ALLEGRO_WITH_ALSAMIDI
AL_VAR(MIDI_DRIVER, midi_alsa);
#define MIDI_DRIVER_ALSA                                          \
      {  MIDI_ALSA,        &midi_alsa,            TRUE  },
#endif /* ALLEGRO_WITH_ALSAMIDI */

	  
#ifdef ALLEGRO_WITH_JACKDIGI
AL_VAR(DIGI_DRIVER, digi_jack);
#define DIGI_DRIVER_JACK                                         \
      {  DIGI_JACK,       &digi_jack,           TRUE  },
#endif /* ALLEGRO_WITH_JACKDIGI */

#endif



/************************************/
/************ X-specific ************/
/************************************/

#define SYSTEM_XWINDOWS          AL_ID('X','W','I','N')

#define KEYBOARD_XWINDOWS        AL_ID('X','W','I','N')
#define MOUSE_XWINDOWS           AL_ID('X','W','I','N')

#define GFX_XWINDOWS             AL_ID('X','W','I','N')
#define GFX_XWINDOWS_FULLSCREEN  AL_ID('X','W','F','S')
#define GFX_XDGA                 AL_ID('X','D','G','A')
#define GFX_XDGA_FULLSCREEN      AL_ID('X','D','F','S')
#define GFX_XDGA2                AL_ID('D','G','A','2')
#define GFX_XDGA2_SOFT           AL_ID('D','G','A','S')


#ifdef ALLEGRO_WITH_XWINDOWS
AL_VAR(SYSTEM_DRIVER, system_xwin);

#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA2
#ifndef ALLEGRO_WITH_MODULES
AL_VAR(GFX_DRIVER, gfx_xdga2);
AL_VAR(GFX_DRIVER, gfx_xdga2_soft);
#endif
#endif

#endif /* ALLEGRO_WITH_XWINDOWS */



/****************************************/
/************ Linux-specific ************/
/****************************************/

#define SYSTEM_LINUX             AL_ID('L','N','X','C')

#define GFX_VGA                  AL_ID('V','G','A',' ')
#define GFX_MODEX                AL_ID('M','O','D','X')
#define GFX_FBCON                AL_ID('F','B',' ',' ')
#define GFX_VBEAF                AL_ID('V','B','A','F')
#define GFX_SVGALIB              AL_ID('S','V','G','A')

#define KEYDRV_LINUX             AL_ID('L','N','X','C')

#define MOUSEDRV_LINUX_PS2       AL_ID('L','P','S','2')
#define MOUSEDRV_LINUX_IPS2      AL_ID('L','I','P','S')
#define MOUSEDRV_LINUX_GPMDATA   AL_ID('G','P','M','D')
#define MOUSEDRV_LINUX_MS        AL_ID('M','S',' ',' ')
#define MOUSEDRV_LINUX_IMS       AL_ID('I','M','S',' ')
#define MOUSEDRV_LINUX_EVDEV     AL_ID('E','V',' ',' ')

#define JOY_TYPE_LINUX_ANALOGUE  AL_ID('L','N','X','A')


#ifdef ALLEGRO_LINUX

AL_VAR(SYSTEM_DRIVER, system_linux);

#ifdef ALLEGRO_LINUX_VGA
   AL_VAR(GFX_DRIVER, gfx_vga);
   AL_VAR(GFX_DRIVER, gfx_modex);
   #define ALLEGRO_GFX_HAS_VGA
#endif

#ifndef ALLEGRO_WITH_MODULES

#ifdef ALLEGRO_LINUX_FBCON
   AL_VAR(GFX_DRIVER, gfx_fbcon);
#endif
#ifdef ALLEGRO_LINUX_SVGALIB
   AL_VAR(GFX_DRIVER, gfx_svgalib);
#endif

#endif

#ifdef ALLEGRO_LINUX_VBEAF
   AL_VAR(GFX_DRIVER, gfx_vbeaf);
   #define ALLEGRO_GFX_HAS_VBEAF
#endif

AL_VAR(MOUSE_DRIVER, mousedrv_linux_ps2);
AL_VAR(MOUSE_DRIVER, mousedrv_linux_ips2);
AL_VAR(MOUSE_DRIVER, mousedrv_linux_gpmdata);
AL_VAR(MOUSE_DRIVER, mousedrv_linux_ms);
AL_VAR(MOUSE_DRIVER, mousedrv_linux_ims);
AL_VAR(MOUSE_DRIVER, mousedrv_linux_evdev);

AL_FUNC_DEPRECATED(void, split_modex_screen, (int lyne));


/* Port I/O functions -- maybe these should be internal */

#ifdef ALLEGRO_LINUX_VGA

#ifdef __cplusplus
extern "C" {
#endif

static INLINE void outportb(unsigned short port, unsigned char value)
{
   __asm__ volatile ("outb %0, %1" : : "a" (value), "d" (port));
}

static INLINE void outportw(unsigned short port, unsigned short value)
{
   __asm__ volatile ("outw %0, %1" : : "a" (value), "d" (port));
}

static INLINE void outportl(unsigned short port, unsigned long value)
{
   __asm__ volatile ("outl %0, %1" : : "a" (value), "d" (port));
}

static INLINE unsigned char inportb(unsigned short port)
{
   unsigned char value;
   __asm__ volatile ("inb %1, %0" : "=a" (value) : "d" (port));
   return value;
}

static INLINE unsigned short inportw(unsigned short port)
{
   unsigned short value;
   __asm__ volatile ("inw %1, %0" : "=a" (value) : "d" (port));
   return value;
}

static INLINE unsigned long inportl(unsigned short port)
{
   unsigned long value;
   __asm__ volatile ("inl %1, %0" : "=a" (value) : "d" (port));
   return value;
}

#ifdef __cplusplus
}
#endif

#endif /* ALLEGRO_LINUX_VGA */


#endif /* ALLEGRO_LINUX */
