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

AL_ARRAY(char, wnd_title);
AL_VAR(int, wnd_x);
AL_VAR(int, wnd_y);
AL_VAR(int, wnd_width);
AL_VAR(int, wnd_height);
AL_VAR(int, wnd_sysmenu);

AL_FUNCPTR(void, user_close_proc, (void));


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


/* input routines */
AL_VAR(int, _win_input_events);
AL_ARRAY(HANDLE, _win_input_event_id);
AL_FUNCPTRARRAY(void, _win_input_event_handler, (void));

AL_FUNC(void, _win_input_init, (int need_thread));
AL_FUNC(void, _win_input_exit, (void));
AL_FUNC(int, _win_input_register_event, (HANDLE event_id, void (*event_handler)(void)));
AL_FUNC(void, _win_input_unregister_event, (HANDLE event_id));


/* keyboard routines */
AL_FUNC(int, key_dinput_acquire, (void));
AL_FUNC(int, key_dinput_unacquire, (void));


/* mouse routines */
AL_VAR(HCURSOR, _win_hcursor);
AL_FUNC(int, mouse_dinput_acquire, (void));
AL_FUNC(int, mouse_dinput_unacquire, (void));
AL_FUNC(int, mouse_dinput_grab, (void));
AL_FUNC(int, mouse_set_syscursor, (void));
AL_FUNC(int, mouse_set_sysmenu, (int state));


/* joystick routines */
#define WINDOWS_MAX_AXES   6

#define WINDOWS_JOYSTICK_INFO_MEMBERS        \
   int caps;                                 \
   int num_axes;                             \
   int axis[WINDOWS_MAX_AXES];               \
   char *axis_name[WINDOWS_MAX_AXES];        \
   int hat;                                  \
   char *hat_name;                           \
   int num_buttons;                          \
   int button[MAX_JOYSTICK_BUTTONS];         \
   char *button_name[MAX_JOYSTICK_BUTTONS];

typedef struct WINDOWS_JOYSTICK_INFO {
   WINDOWS_JOYSTICK_INFO_MEMBERS
} WINDOWS_JOYSTICK_INFO;

AL_FUNC(int, win_add_joystick, (WINDOWS_JOYSTICK_INFO *win_joy));
AL_FUNC(void, win_remove_all_joysticks, (void));
AL_FUNC(int, win_update_joystick_status, (int n, WINDOWS_JOYSTICK_INFO *win_joy));

AL_FUNC(int, joystick_dinput_acquire, (void));
AL_FUNC(int, joystick_dinput_unacquire, (void));


/* thread routines */
AL_FUNC(void, _win_thread_init, (void));
AL_FUNC(void, _win_thread_exit, (void));


/* synchronization routines */
AL_FUNC(void *, sys_directx_create_mutex, (void));
AL_FUNC(void, sys_directx_destroy_mutex, (void *handle));
AL_FUNC(void, sys_directx_lock_mutex, (void *handle));
AL_FUNC(void, sys_directx_unlock_mutex, (void *handle));


/* sound routines */
AL_FUNC(_DRIVER_INFO *, _get_win_digi_driver_list, (void));
AL_FUNC(void, _free_win_digi_driver_list, (void));

AL_FUNC(DIGI_DRIVER *, _get_dsalmix_driver, (char *name, LPGUID guid, int num));
AL_FUNC(void, _free_win_dsalmix_name_list, (void));
AL_FUNC(DIGI_DRIVER *, _get_woalmix_driver, (int num));

AL_FUNC(int, digi_directsound_capture_init, (LPGUID guid));
AL_FUNC(void, digi_directsound_capture_exit, (void));
AL_FUNC(int, digi_directsound_capture_detect, (LPGUID guid));
AL_FUNC(int, digi_directsound_rec_cap_rate, (int bits, int stereo));
AL_FUNC(int, digi_directsound_rec_cap_param, (int rate, int bits, int stereo));
AL_FUNC(int, digi_directsound_rec_source, (int source));
AL_FUNC(int, digi_directsound_rec_start, (int rate, int bits, int stereo));
AL_FUNC(void, digi_directsound_rec_stop, (void));
AL_FUNC(int, digi_directsound_rec_read, (void *buf));


/* midi routines */
AL_FUNC(_DRIVER_INFO *, _get_win_midi_driver_list, (void));
AL_FUNC(void, _free_win_midi_driver_list, (void));

AL_FUNC(void, midi_switch_out, (void));


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

