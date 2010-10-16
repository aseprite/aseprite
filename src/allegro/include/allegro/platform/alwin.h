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
 *      Windows-specific header defines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_WINDOWS
   #error bad include
#endif



/*******************************************/
/********** magic main emulation ***********/
/*******************************************/
#ifdef __cplusplus
extern "C" {
#endif

AL_FUNC(int, _WinMain, (void *_main, void *hInst, void *hPrev, char *Cmd, int nShow));

#ifdef __cplusplus
}
#endif


#if (!defined ALLEGRO_NO_MAGIC_MAIN) && (!defined ALLEGRO_SRC)

   #define ALLEGRO_MAGIC_MAIN
   #define main _mangled_main
   #undef END_OF_MAIN

   /* disable strict pointer typing because of the vague prototype below */
   #define NO_STRICT

   #ifdef __cplusplus
      extern "C" int __stdcall WinMain(void *hInst, void *hPrev, char *Cmd, int nShow);
   #endif

   #define END_OF_MAIN()                                                     \
                                                                             \
      int __stdcall WinMain(void *hInst, void *hPrev, char *Cmd, int nShow)  \
      {                                                                      \
         return _WinMain((void *)_mangled_main, hInst, hPrev, Cmd, nShow);   \
      }

#endif



/*******************************************/
/************* system drivers **************/
/*******************************************/
#define SYSTEM_DIRECTX           AL_ID('D','X',' ',' ')

AL_VAR(SYSTEM_DRIVER, system_directx);



/*******************************************/
/************** timer drivers **************/
/*******************************************/
#define TIMER_WIN32_HIGH_PERF    AL_ID('W','3','2','H')
#define TIMER_WIN32_LOW_PERF     AL_ID('W','3','2','L')



/*******************************************/
/************ keyboard drivers *************/
/*******************************************/
#define KEYBOARD_DIRECTX         AL_ID('D','X',' ',' ')



/*******************************************/
/************* mouse drivers ***************/
/*******************************************/
#define MOUSE_DIRECTX            AL_ID('D','X',' ',' ')



/*******************************************/
/*************** gfx drivers ***************/
/*******************************************/
#define GFX_DIRECTX              AL_ID('D','X','A','C')
#define GFX_DIRECTX_ACCEL        AL_ID('D','X','A','C')
#define GFX_DIRECTX_SAFE         AL_ID('D','X','S','A')
#define GFX_DIRECTX_SOFT         AL_ID('D','X','S','O')
#define GFX_DIRECTX_WIN          AL_ID('D','X','W','N')
#define GFX_DIRECTX_OVL          AL_ID('D','X','O','V')
#define GFX_GDI                  AL_ID('G','D','I','B')

AL_VAR(GFX_DRIVER, gfx_directx_accel);
AL_VAR(GFX_DRIVER, gfx_directx_safe);
AL_VAR(GFX_DRIVER, gfx_directx_soft);
AL_VAR(GFX_DRIVER, gfx_directx_win);
AL_VAR(GFX_DRIVER, gfx_directx_ovl);
AL_VAR(GFX_DRIVER, gfx_gdi);

#define GFX_DRIVER_DIRECTX                                              \
   {  GFX_DIRECTX_ACCEL,   &gfx_directx_accel,     TRUE  },             \
   {  GFX_DIRECTX_SOFT,    &gfx_directx_soft,      TRUE  },             \
   {  GFX_DIRECTX_SAFE,    &gfx_directx_safe,      TRUE  },             \
   {  GFX_DIRECTX_WIN,     &gfx_directx_win,       TRUE  },             \
   {  GFX_DIRECTX_OVL,     &gfx_directx_ovl,       TRUE  },             \
   {  GFX_GDI,             &gfx_gdi,               FALSE },



/********************************************/
/*************** sound drivers **************/
/********************************************/
#define DIGI_DIRECTX(n)          AL_ID('D','X','A'+(n),' ')
#define DIGI_DIRECTAMX(n)        AL_ID('A','X','A'+(n),' ')
#define DIGI_WAVOUTID(n)         AL_ID('W','O','A'+(n),' ')
#define MIDI_WIN32MAPPER         AL_ID('W','3','2','M')
#define MIDI_WIN32(n)            AL_ID('W','3','2','A'+(n))
#define MIDI_WIN32_IN(n)         AL_ID('W','3','2','A'+(n))



/*******************************************/
/************ joystick drivers *************/
/*******************************************/
#define JOY_TYPE_DIRECTX         AL_ID('D','X',' ',' ')
#define JOY_TYPE_WIN32           AL_ID('W','3','2',' ')

AL_VAR(JOYSTICK_DRIVER, joystick_directx);
AL_VAR(JOYSTICK_DRIVER, joystick_win32);

#define JOYSTICK_DRIVER_DIRECTX                                   \
      { JOY_TYPE_DIRECTX,        &joystick_directx,  TRUE  },

#define JOYSTICK_DRIVER_WIN32                                     \
      { JOY_TYPE_WIN32,          &joystick_win32,  TRUE  },
