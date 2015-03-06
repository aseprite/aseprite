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
 *      Main system driver for the Windows library.
 *
 *      By Stefan Schimanski.
 *
 *      Window close button support by Javier Gonzalez.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"
#include "utf8.h"

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif

/* DMC requires a DllMain() function, or else the DLL hangs. */
#ifndef ALLEGRO_STATICLINK
BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason, LPVOID lpReserved)
{
   return TRUE;
}
#endif


static int sys_directx_init(void);
static void sys_directx_exit(void);
static void sys_directx_get_executable_name(char *output, int size);
static void sys_directx_set_window_title(AL_CONST char *name);
static int sys_directx_set_close_button_callback(void (*proc)(void));
static int sys_directx_set_resize_callback(void (*proc)(RESIZE_DISPLAY_EVENT *ev));
static void sys_directx_message(AL_CONST char *msg);
static void sys_directx_assert(AL_CONST char *msg);
static void sys_directx_save_console_state(void);
static void sys_directx_restore_console_state(void);
static int sys_directx_desktop_color_depth(void);
static int sys_directx_get_desktop_resolution(int *width, int *height);
static void sys_directx_yield_timeslice(void);
static int sys_directx_trace_handler(AL_CONST char *msg);


/* the main system driver for running under DirectX */
SYSTEM_DRIVER system_directx =
{
   SYSTEM_DIRECTX,
   empty_string,                /* char *name; */
   empty_string,                /* char *desc; */
   "DirectX",
   sys_directx_init,
   sys_directx_exit,
   sys_directx_get_executable_name,
   NULL,                        /* AL_METHOD(int, find_resource, (char *dest, char *resource, int size)); */
   sys_directx_set_window_title,
   sys_directx_set_close_button_callback,
   sys_directx_set_resize_callback,
   sys_directx_message,
   sys_directx_assert,
   sys_directx_save_console_state,
   sys_directx_restore_console_state,
   NULL,                        /* AL_METHOD(struct BITMAP *, create_bitmap, (int color_depth, int width, int height)); */
   NULL,                        /* AL_METHOD(void, created_bitmap, (struct BITMAP *bmp)); */
   NULL,                        /* AL_METHOD(struct BITMAP *, create_sub_bitmap, (struct BITMAP *parent, int x, int y, int width, int height)); */
   NULL,                        /* AL_METHOD(void, created_sub_bitmap, (struct BITMAP *bmp)); */
   NULL,                        /* AL_METHOD(int, destroy_bitmap, (struct BITMAP *bitmap)); */
   NULL,                        /* AL_METHOD(void, read_hardware_palette, (void)); */
   NULL,                        /* AL_METHOD(void, set_palette_range, (PALETTE p, int from, int to, int vsync)); */
   NULL,                        /* AL_METHOD(struct GFX_VTABLE *, get_vtable, (int color_depth)); */
   sys_directx_set_display_switch_mode,
   NULL,                        /* AL_METHOD(void, display_switch_lock, (int lock)); */
   sys_directx_desktop_color_depth,
   sys_directx_get_desktop_resolution,
   NULL,                        /* get_gfx_safe_mode */
   sys_directx_yield_timeslice,
   sys_directx_create_mutex,
   sys_directx_destroy_mutex,
   sys_directx_lock_mutex,
   sys_directx_unlock_mutex,
   NULL,                        /* AL_METHOD(_DRIVER_INFO *, gfx_drivers, (void)); */
   NULL,                        /* AL_METHOD(_DRIVER_INFO *, keyboard_drivers, (void)); */
   NULL,                        /* AL_METHOD(_DRIVER_INFO *, mouse_drivers, (void)); */
   NULL                         /* AL_METHOD(_DRIVER_INFO *, timer_drivers, (void)); */
};

static char sys_directx_desc[64] = EMPTY_STRING;


_DRIVER_INFO _system_driver_list[] =
{
   {SYSTEM_DIRECTX, &system_directx, TRUE},
   {SYSTEM_NONE, &system_none, FALSE},
   {0, NULL, 0}
};


/* general vars */
HINSTANCE allegro_inst = NULL;
HANDLE allegro_thread = NULL;
CRITICAL_SECTION allegro_critical_section;
int _dx_ver;

/* internals */
static RECT wnd_rect;



/* sys_directx_init:
 *  Top level system driver wakeup call.
 */
static int sys_directx_init(void)
{
   char tmp[64];
   unsigned long win_ver;
   HANDLE current_thread;
   HANDLE current_process;

   /* init thread */
   _win_thread_init();

   allegro_inst = GetModuleHandle(NULL);

   /* get allegro thread handle */
   current_thread = GetCurrentThread();
   current_process = GetCurrentProcess();
   DuplicateHandle(current_process, current_thread,
                   current_process, &allegro_thread,
                   0, FALSE, DUPLICATE_SAME_ACCESS);

   /* get versions */
   win_ver = GetVersion();
   os_version = win_ver & 0xFF;
   os_revision = (win_ver & 0xFF00) >> 8;
   os_multitasking = TRUE;

   if (win_ver < 0x80000000) {

      /* Since doesn't exist os_version == 7 or greater yet,
         these will be detected as Vista instead of NT. */
      if (os_version >= 6) {
         os_type = OSTYPE_WINVISTA;
      }
      else if (os_version == 5) {
         /* If in the future a os_revision == 3 or greater comes,
            it will be detected as Win2003 instead of Win2000. */
         if (os_revision >= 2)
            os_type = OSTYPE_WIN2003;
         else if (os_revision == 1)
            os_type = OSTYPE_WINXP;
         else
            os_type = OSTYPE_WIN2000;
      }
      else
         os_type = OSTYPE_WINNT;
   }
   else if (os_version == 4) {
      if (os_revision == 90)
         os_type = OSTYPE_WINME;
      else if (os_revision == 10)
         os_type = OSTYPE_WIN98;
      else
         os_type = OSTYPE_WIN95;
   }
   else
      os_type = OSTYPE_WIN3;

   _dx_ver = get_dx_ver();

   uszprintf(sys_directx_desc, sizeof(sys_directx_desc),
             uconvert_ascii("DirectX %u.%x", tmp), _dx_ver >> 8, _dx_ver & 0xff);
   system_directx.desc = sys_directx_desc;

   /* setup general critical section */
   InitializeCriticalSection(&allegro_critical_section);

   /* install a Windows specific trace handler */
   if (!_al_trace_handler)
      register_trace_handler(sys_directx_trace_handler);

   /* setup the display switch system */
   sys_directx_display_switch_init();

   /* either use a user window or create a new window */
   if (init_directx_window() != 0)
      goto Error;

   return 0;

 Error:
   sys_directx_exit();

   return -1;
}



/* sys_directx_exit:
 *  The end of the world...
 */
static void sys_directx_exit(void)
{
   /* unhook or close window */
   exit_directx_window();

   /* shutdown display switch system */
   sys_directx_display_switch_exit();

   /* remove resources */
   DeleteCriticalSection(&allegro_critical_section);

   /* shutdown thread */
   _win_thread_exit();

   allegro_inst = NULL;
}



/* sys_directx_get_executable_name:
 *  Returns full path to the current executable.
 */
static void sys_directx_get_executable_name(char *output, int size)
{
   wchar_t* temp = _AL_MALLOC_ATOMIC(size);

   if (GetModuleFileName(allegro_inst, temp, size))
      do_uconvert((char*)temp, U_UNICODE, output, U_CURRENT, size);
   else
      usetc(output, 0);

   _AL_FREE(temp);
}



/* sys_directx_set_window_title:
 *  Alters the application title.
 */
static void sys_directx_set_window_title(AL_CONST char *name)
{
   HWND allegro_wnd = win_get_window();

   do_uconvert(name, U_CURRENT, (char*)wnd_title, U_UNICODE, WND_TITLE_SIZE);
   SetWindowText(allegro_wnd, wnd_title);
}



/* sys_directx_set_close_button_callback:
 *  Sets the close button callback function.
 */
static int sys_directx_set_close_button_callback(void (*proc)(void))
{
   DWORD class_style;
   HMENU sys_menu;
   HWND allegro_wnd = win_get_window();

   user_close_proc = proc;

   /* get the old class style */
   class_style = GetClassLong(allegro_wnd, GCL_STYLE);

   /* and the system menu handle */
   sys_menu = GetSystemMenu(allegro_wnd, FALSE);

   /* enable or disable the no_close_button flag and the close menu option */
   if (proc) {
      class_style &= ~CS_NOCLOSE;
      EnableMenuItem(sys_menu, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);
   }
   else {
      class_style |= CS_NOCLOSE;
      EnableMenuItem(sys_menu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
   }

   /* change the class to the new style */
   SetClassLong(allegro_wnd, GCL_STYLE, class_style);

   /* Redraw the whole window to display the changes of the button.
    * (we use this because UpdateWindow() only works for the client area)
    */
   RedrawWindow(allegro_wnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);

   return 0;
}


static int sys_directx_set_resize_callback(void (*proc)(RESIZE_DISPLAY_EVENT *ev))
{
   user_resize_proc = proc;
   return 0;
}



/* sys_directx_message:
 *  Displays a message.
 */
static void sys_directx_message(AL_CONST char *msg)
{
   char *tmp1 = _AL_MALLOC_ATOMIC(ALLEGRO_MESSAGE_SIZE);
   char tmp2[WND_TITLE_SIZE*2];
   HWND allegro_wnd = win_get_window();

   while ((ugetc(msg) == '\r') || (ugetc(msg) == '\n'))
      msg += uwidth(msg);

   MessageBoxW(allegro_wnd,
               (wchar_t*)uconvert(msg, U_CURRENT, tmp1, U_UNICODE, ALLEGRO_MESSAGE_SIZE),
               (wchar_t*)uconvert((char*)wnd_title, U_CURRENT, tmp2, U_UNICODE, sizeof(tmp2)),
               MB_OK);

   _AL_FREE(tmp1);
}



/* sys_directx_assert
 *  Handles assertions.
 */
static void sys_directx_assert(AL_CONST char *msg)
{
   OutputDebugStringA(msg);  /* thread safe */
   DebugBreak();
}



/* sys_directx_save_console_state:
 *  Saves console window size and position.
 */
static void sys_directx_save_console_state(void)
{
   HWND allegro_wnd = win_get_window();
   GetWindowRect(allegro_wnd, &wnd_rect);
}



/* sys_directx_restore_console_state:
 *  Restores console window size and position.
 */
static void sys_directx_restore_console_state(void)
{
   HWND allegro_wnd = win_get_window();

   /* reset switch mode */
   _win_reset_switch_mode();

   /* re-size and hide window */
   SetWindowPos(allegro_wnd, HWND_TOP, wnd_rect.left, wnd_rect.top,
                wnd_rect.right - wnd_rect.left, wnd_rect.bottom - wnd_rect.top,
                SWP_NOCOPYBITS);
   ShowWindow(allegro_wnd, SW_SHOW);
}



/* sys_directx_desktop_color_depth:
 *  Returns the current desktop color depth.
 */
static int sys_directx_desktop_color_depth(void)
{
   /* The regular way of retrieving the desktop
    * color depth is broken under Windows 95:
    *
    *   DEVMODE display_mode;
    *
    *   display_mode.dmSize = sizeof(DEVMODE);
    *   display_mode.dmDriverExtra = 0;
    *   if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &display_mode) == 0)
    *      return 0;
    *
    *   return display_mode.dmBitsPerPel;
    */

   HDC dc;
   int depth;

   dc = GetDC(NULL);
   depth = GetDeviceCaps(dc, BITSPIXEL);
   ReleaseDC(NULL, dc);

   return depth;
}



/* sys_directx_get_desktop_resolution:
 *  Returns the current desktop resolution.
 */
static int sys_directx_get_desktop_resolution(int *width, int *height)
{
   /* same thing for the desktop resolution:
    *
    *   DEVMODE display_mode;
    *
    *   display_mode.dmSize = sizeof(DEVMODE);
    *   display_mode.dmDriverExtra = 0;
    *   if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &display_mode) == 0)
    *      return -1;
    *
    *   *width  = display_mode.dmPelsWidth;
    *   *height = display_mode.dmPelsHeight;
    *
    *   return 0;
    */

   HDC dc;

   dc = GetDC(NULL);
   *width  = GetDeviceCaps(dc, HORZRES);
   *height = GetDeviceCaps(dc, VERTRES);
   ReleaseDC(NULL, dc);

   return 0;
}



/* sys_directx_yield_timeslice:
 *  Yields remaining timeslice portion to the system.
 */
static void sys_directx_yield_timeslice(void)
{
   Sleep(0);
}



/* sys_directx_trace_handler
 *  Handles trace output.
 */
static int sys_directx_trace_handler(AL_CONST char *msg)
{
   OutputDebugStringA(msg);  /* thread safe */
   return 0;
}



/* _WinMain:
 *  Entry point for Windows GUI programs, hooked by a macro in alwin.h,
 *  which makes it look as if the application can still have a normal
 *  main() function.
 */
int _WinMain(void* _main, void* hInst, void* hPrev, char* Cmd, int nShow)
{
   int (*mainfunc)(int argc, char *argv[]) = (int (*)(int, char *[]))_main;
   wchar_t* argbuf;
   wchar_t* cmdline;
   wchar_t* arg_w;
   char** argv;
   int argc;
   int argc_max;
   int i, j, q;

   /* can't use parameter because it doesn't include the executable name */
   cmdline = GetCommandLine();
   i = (int)(sizeof(wchar_t) * (wcslen(cmdline) + 1));
   argbuf = _AL_MALLOC(i);
   memcpy(argbuf, cmdline, i);

   argc = 0;
   argc_max = 64;
   argv = _AL_MALLOC(sizeof(char*) * argc_max);
   if (!argv) {
      _AL_FREE(argbuf);
      return 1;
   }

   i = 0;

   /* parse commandline into argc/argv format */
   while (argbuf[i]) {
      while ((argbuf[i]) && (iswspace(argbuf[i])))
         i++;

      if (argbuf[i]) {
         if ((argbuf[i] == L'\'') || (argbuf[i] == L'"')) {
            q = argbuf[i++];
            if (!argbuf[i])
               break;
         }
         else
            q = 0;

         arg_w = argbuf+i;
         ++argc;

         if (argc >= argc_max) {
            argc_max += 64;
            argv = _AL_REALLOC(argv, sizeof(char *) * argc_max);
            if (!argv) {
               _AL_FREE(argbuf);
               return 1;
            }
         }

         while ((argbuf[i]) && ((q) ? (argbuf[i] != q) : (!iswspace(argbuf[i]))))
            i++;

         if (argbuf[i]) {
            argbuf[i] = 0;
            i++;
         }

         argv[argc-1] = convert_widechar_to_utf8(arg_w);
      }
   }

   argv[argc] = NULL;

   _AL_FREE(argbuf);

   /* call the application entry point */
   i = mainfunc(argc, argv);

   for (j=0; j<argc; ++j)
     _AL_FREE(argv[j]);
   _AL_FREE(argv);

   return i;
}



/* win_err_str:
 *  Returns a error string for a window error.
 */
char *win_err_str(long err)
{
   static char msg[256];

   FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPSTR)&msg[0], 0, NULL);

   return msg;
}



/* thread_safe_trace:
 *  Outputs internal trace message.
 */
void thread_safe_trace(char *msg,...)
{
   char buf[256];
   va_list ap;

   /* todo, some day: use vsnprintf (C99) */
   va_start(ap, msg);
   vsprintf(buf, msg, ap);
   va_end(ap);

   OutputDebugStringA(buf);  /* thread safe */
}
