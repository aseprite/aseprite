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
 *      Some definitions for internal use by the Windows library code.
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */


#ifndef AINTWIN_H
#define AINTWIN_H

#ifndef ALLEGRO_H
   #error must include allegro.h first
#endif

#ifndef ALLEGRO_WINDOWS
   #error bad include
#endif


#include "winalleg.h"

#ifndef SCAN_DEPEND
   /* workaround for buggy MinGW32 headers */
   #ifdef ALLEGRO_MINGW32
      #ifndef HMONITOR_DECLARED
         #define HMONITOR_DECLARED
      #endif
      #if (defined _HRESULT_DEFINED) && (defined WINNT)
         #undef WINNT
      #endif
   #endif

   #include <objbase.h>  /* for LPGUID */
#endif


#ifdef __cplusplus
   extern "C" {
#endif

/* generals */
AL_VAR(HINSTANCE, allegro_inst);
AL_VAR(HANDLE, allegro_thread);
AL_VAR(CRITICAL_SECTION, allegro_critical_section);
AL_VAR(int, _dx_ver);

#define _enter_critical()  EnterCriticalSection(&allegro_critical_section)
#define _exit_critical()   LeaveCriticalSection(&allegro_critical_section)

AL_FUNC(int, init_directx_window, (void));
AL_FUNC(void, exit_directx_window, (void));
AL_FUNC(int, get_dx_ver, (void));
AL_FUNC(int, adjust_window, (int w, int h));
AL_FUNC(void, restore_window_style, (void));
AL_FUNC(void, save_window_pos, (void));


/* main window */
#define WND_TITLE_SIZE  128

AL_ARRAY(wchar_t, wnd_title);
AL_VAR(int, wnd_x);
AL_VAR(int, wnd_y);
AL_VAR(int, wnd_width);
AL_VAR(int, wnd_height);
AL_VAR(int, wnd_sysmenu);

AL_FUNCPTR(void, user_close_proc, (void));
AL_FUNCPTR(void, user_resize_proc, (RESIZE_DISPLAY_EVENT *ev));


/* gfx synchronization */
AL_VAR(CRITICAL_SECTION, gfx_crit_sect);
AL_VAR(int, gfx_crit_sect_nesting);

#define _enter_gfx_critical()  EnterCriticalSection(&gfx_crit_sect); \
                               gfx_crit_sect_nesting++
#define _exit_gfx_critical()   LeaveCriticalSection(&gfx_crit_sect); \
                               gfx_crit_sect_nesting--
#define GFX_CRITICAL_RELEASED  (!gfx_crit_sect_nesting)


/* switch routines */
AL_VAR(int, _win_app_foreground);

AL_FUNC(void, sys_directx_display_switch_init, (void));
AL_FUNC(void, sys_directx_display_switch_exit, (void));
AL_FUNC(int, sys_directx_set_display_switch_mode, (int mode));

AL_FUNC(void, _win_switch_in, (void));
AL_FUNC(void, _win_switch_out, (void));
AL_FUNC(void, _win_reset_switch_mode, (void));

AL_FUNC(int, _win_thread_switch_out, (void));


/* main window routines */
AL_FUNC(int, wnd_call_proc, (int (*proc)(void)));
AL_FUNC(void, wnd_schedule_proc, (int (*proc)(void)));


/* keyboard routines */
AL_FUNC(void, _al_win_kbd_update_shifts, (void));
AL_FUNC(void, _al_win_kbd_handle_key_press, (int scode, int vcode, BOOL repeated));
AL_FUNC(void, _al_win_kbd_handle_key_release, (int vcode));


/* mouse routines */
AL_VAR(HCURSOR, _win_hcursor);
AL_FUNC(int, mouse_set_syscursor, (void));
AL_FUNC(void, _al_win_mouse_handle_button, (HWND hwnd, int button, BOOL down, int x, int y, BOOL abs));
AL_FUNC(void, _al_win_mouse_handle_wheel, (HWND hwnd, int z, BOOL abs));
AL_FUNC(void, _al_win_mouse_handle_move, (HWND hwnd, int x, int y));


/* thread routines */
AL_FUNC(void, _win_thread_init, (void));
AL_FUNC(void, _win_thread_exit, (void));


/* synchronization routines */
AL_FUNC(void *, sys_directx_create_mutex, (void));
AL_FUNC(void, sys_directx_destroy_mutex, (void *handle));
AL_FUNC(void, sys_directx_lock_mutex, (void *handle));
AL_FUNC(void, sys_directx_unlock_mutex, (void *handle));


/* file routines */
AL_VAR(int, _al_win_unicode_filenames);


/* error handling */
AL_FUNC(char* , win_err_str, (long err));
AL_FUNC(void, thread_safe_trace, (char *msg, ...));

#if DEBUGMODE >= 2
   #define _TRACE                 thread_safe_trace
#else
   #define _TRACE                 1 ? (void) 0 : thread_safe_trace
#endif


#ifdef __cplusplus
   }
#endif

#endif          /* !defined AINTWIN_H */
