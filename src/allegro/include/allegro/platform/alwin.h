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
/*************** gfx drivers ***************/
/*******************************************/
#define GFX_DIRECTX              AL_ID('D','X','A','C')
#define GFX_DIRECTX_WIN          AL_ID('D','X','W','N')

AL_VAR(GFX_DRIVER, gfx_directx_win);

#define GFX_DRIVER_DIRECTX                                              \
   {  GFX_DIRECTX_WIN,     &gfx_directx_win,       TRUE  },
