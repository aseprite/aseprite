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
 *      QNX specific driver definitions.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


/* magic to capture name of executable file */
extern int    __crt0_argc;
extern char **__crt0_argv;

#ifndef ALLEGRO_NO_MAGIC_MAIN
   #define ALLEGRO_MAGIC_MAIN
   #define main _mangled_main
   #undef END_OF_MAIN
   #define END_OF_MAIN() void *_mangled_main_address = (void*) _mangled_main;
#else
   #undef END_OF_MAIN
   #define END_OF_MAIN() void *_mangled_main_address;
#endif


/* System driver */
#define SYSTEM_QNX            AL_ID('Q','S','Y','S')
AL_VAR(SYSTEM_DRIVER, system_qnx);


/* Timer driver */
#define TIMERDRV_UNIX_PTHREADS	AL_ID('P','T','H','R')
AL_VAR(TIMER_DRIVER, timerdrv_unix_pthreads);


/* Keyboard driver */
#define KEYBOARD_QNX          AL_ID('Q','K','E','Y')
AL_VAR(KEYBOARD_DRIVER, keyboard_qnx);


/* Mouse driver */
#define MOUSE_QNX             AL_ID('Q','M','S','E')
AL_VAR(MOUSE_DRIVER, mouse_qnx);


/* Graphics drivers */
#define GFX_PHOTON            AL_ID('Q','P','A','C')
#define GFX_PHOTON_ACCEL      AL_ID('Q','P','A','C')
#define GFX_PHOTON_SOFT       AL_ID('Q','P','S','O')
#define GFX_PHOTON_SAFE       AL_ID('Q','P','S','A')
#define GFX_PHOTON_WIN        AL_ID('Q','P','W','N')
AL_VAR(GFX_DRIVER, gfx_photon);
AL_VAR(GFX_DRIVER, gfx_photon_accel);
AL_VAR(GFX_DRIVER, gfx_photon_soft);
AL_VAR(GFX_DRIVER, gfx_photon_safe);
AL_VAR(GFX_DRIVER, gfx_photon_win);


/* Sound driver */
#define DIGI_ALSA             AL_ID('A','L','S','A')
AL_VAR(DIGI_DRIVER, digi_alsa);


/* MIDI driver */
#define MIDI_ALSA             AL_ID('A','M','I','D')
AL_VAR(MIDI_DRIVER, midi_alsa);
