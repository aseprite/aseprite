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
 *      MacOS X specific header defines.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALOSX_H
#define ALOSX_H

#ifndef ALLEGRO_MACOSX
   #error bad include
#endif


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <fcntl.h>
   #include <unistd.h>
   #include <signal.h>
   #include <pthread.h>
   #if defined __OBJC__ && defined ALLEGRO_SRC
      #undef TRUE
      #undef FALSE
      #import <mach/mach.h>
      #import <mach/mach_error.h>
      #import <AppKit/AppKit.h>
      #import <ApplicationServices/ApplicationServices.h>
      #import <Cocoa/Cocoa.h>
      #import <CoreAudio/CoreAudio.h>
      #import <AudioUnit/AudioUnit.h>
      #import <AudioToolbox/AudioToolbox.h>
      #import <QuickTime/QuickTime.h>
      #import <IOKit/IOKitLib.h>
      #import <IOKit/IOCFPlugIn.h>
      #import <IOKit/hid/IOHIDLib.h>
      #import <IOKit/hid/IOHIDKeys.h>
      #import <Kernel/IOKit/hidsystem/IOHIDUsageTables.h>
      #undef TRUE
      #undef FALSE
      #undef assert
      #define TRUE  -1
      #define FALSE 0
   #endif
#endif


/* The following code comes from alunix.h */
/* Magic to capture name of executable file */
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
#define SYSTEM_MACOSX           AL_ID('O','S','X',' ')
AL_VAR(SYSTEM_DRIVER, system_macosx);

/* Timer driver */
#define TIMERDRV_UNIX_PTHREADS  AL_ID('P','T','H','R')
AL_VAR(TIMER_DRIVER, timerdrv_unix_pthreads);

/* Keyboard driver */
#define KEYBOARD_MACOSX         AL_ID('O','S','X','K')
AL_VAR(KEYBOARD_DRIVER, keyboard_macosx);

/* Mouse driver */
#define MOUSE_MACOSX            AL_ID('O','S','X','M')
AL_VAR(MOUSE_DRIVER, mouse_macosx);

/* Gfx drivers */
#define GFX_QUARTZ_WINDOW       AL_ID('Q','Z','W','N')
#define GFX_QUARTZ_FULLSCREEN   AL_ID('Q','Z','F','L')
AL_VAR(GFX_DRIVER, gfx_quartz_window);
AL_VAR(GFX_DRIVER, gfx_quartz_full);

/* Digital sound drivers */
#define DIGI_CORE_AUDIO         AL_ID('D','C','A',' ')
#define DIGI_SOUND_MANAGER      AL_ID('S','N','D','M')
AL_VAR(DIGI_DRIVER, digi_core_audio);
AL_VAR(DIGI_DRIVER, digi_sound_manager);

/* MIDI music drivers */
#define MIDI_CORE_AUDIO         AL_ID('M','C','A',' ')
#define MIDI_QUICKTIME          AL_ID('Q','T','M',' ')
AL_VAR(MIDI_DRIVER, midi_core_audio);
AL_VAR(MIDI_DRIVER, midi_quicktime);

/* Joystick drivers */
#define JOYSTICK_HID            AL_ID('H','I','D','J')
AL_VAR(JOYSTICK_DRIVER, joystick_hid);


#endif
