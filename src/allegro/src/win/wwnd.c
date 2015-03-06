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
 *      Main window creation and management.
 *
 *      By Stefan Schimanski.
 *
 *      Added resize support by David Capello.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"

#include <windows.h>
#include <windowsx.h>

#include <string.h>
#include <process.h>
#include <time.h>

#ifndef ALLEGRO_WINDOWS
   #error something is wrong with the makefile
#endif

#define PREFIX_I                "al-wwnd INFO: "
#define PREFIX_W                "al-wwnd WARNING: "
#define PREFIX_E                "al-wwnd ERROR: "


/* general */
static HWND allegro_wnd = NULL;
wchar_t wnd_title[WND_TITLE_SIZE];  /* Unicode string title */
int wnd_x = 0;
int wnd_y = 0;
int wnd_width = 0;
int wnd_height = 0;
int wnd_sysmenu = FALSE;

static int last_wnd_x = -1;
static int last_wnd_y = -1;

/* graphics */
WIN_GFX_DRIVER *win_gfx_driver;
CRITICAL_SECTION gfx_crit_sect;
int gfx_crit_sect_nesting = 0;

/* close button user hook */
void (*user_close_proc)(void) = NULL;

/* TRUE if the user resized the window and user_resize_proc hook
   should be called. */
static BOOL sizing = FALSE;

/* resize user hook (called when the windows is resized */
void (*user_resize_proc)(RESIZE_DISPLAY_EVENT *ev) = NULL;

/* window thread internals */
#define ALLEGRO_WND_CLASS L"AsepriteWindowClass"
static HANDLE wnd_thread = NULL;
static int old_style = 0;

/* custom window msgs */
static UINT msg_call_proc = 0;
static UINT msg_suicide = 0;

/* Used in adjust_window(). */
#ifndef CCHILDREN_TITLEBAR
   #define CCHILDREN_TITLEBAR 5
typedef struct {
   DWORD cbSize;
   RECT  rcTitleBar;
   DWORD rgstate[CCHILDREN_TITLEBAR + 1];
} TITLEBARINFO, *PTITLEBARINFO, *LPTITLEBARINFO;
#endif /* ifndef CCHILDREN_TITLEBAR */



/* wnd_call_proc:
 *  Instructs the window thread to call the specified procedure and
 *  waits until the procedure has returned.
 */
int wnd_call_proc(int (*proc) (void))
{
   return SendMessage(allegro_wnd, msg_call_proc, (WPARAM)proc, 0);
}



/* wnd_schedule_proc:
 *  Instructs the window thread to call the specified procedure but
 *  doesn't wait until the procedure has returned.
 */
void wnd_schedule_proc(int (*proc) (void))
{
   PostMessage(allegro_wnd, msg_call_proc, (WPARAM)proc, 0);
}



/* directx_wnd_proc:
 *  Window procedure for the Allegro window class.
 */
static LRESULT CALLBACK directx_wnd_proc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam)
{
   if (message == msg_call_proc)
      return ((int (*)(void))wparam) ();

   if (message == msg_suicide) {
      DestroyWindow(wnd);
      return 0;
   }

   switch (message) {

      case WM_CREATE:
         allegro_wnd = wnd;
         break;

      case WM_DESTROY:
         PostQuitMessage(0);
         allegro_wnd = NULL;
         break;

      case WM_SETCURSOR:
         if (win_gfx_driver && _mouse_installed) {
            int hittest = LOWORD(lparam);
            if (hittest == HTCLIENT) {
               mouse_set_syscursor();
               return 1;  /* not TRUE */
            }
         }
         break;

      case WM_ACTIVATE:
         if (LOWORD(wparam) == WA_INACTIVE) {
            _win_switch_out();
         }
         else {
            /* Ignore the WM_ACTIVATE event if the window is minimized. */
            if (HIWORD(wparam))
               break;

            /* no delay in windowed mode */
            _win_switch_in();
         }
         break;

      case WM_ENTERSIZEMOVE:
         if (win_gfx_driver && win_gfx_driver->enter_sysmode)
            win_gfx_driver->enter_sysmode();
         break;

      case WM_EXITSIZEMOVE:
         if (win_gfx_driver && win_gfx_driver->exit_sysmode)
            win_gfx_driver->exit_sysmode();
         break;

      case WM_MOVE:
         if (GetActiveWindow() == allegro_wnd) {
            if (!IsIconic(allegro_wnd)) {
               wnd_x = (short) LOWORD(lparam);
               wnd_y = (short) HIWORD(lparam);

               if (win_gfx_driver && win_gfx_driver->move)
                  win_gfx_driver->move(wnd_x, wnd_y, wnd_width, wnd_height);
            }
            else if (win_gfx_driver && win_gfx_driver->iconify) {
               win_gfx_driver->iconify();
            }
         }
         break;

      case WM_SIZE: {
         int old_width = wnd_width;
         int old_height = wnd_height;

         wnd_width = LOWORD(lparam);
         wnd_height = HIWORD(lparam);

         if ((wnd_width > 0 && wnd_height > 0) &&
             (sizing || (wparam == SIZE_MAXIMIZED ||
                         wparam == SIZE_RESTORED))) {
            sizing = FALSE;
            if (user_resize_proc) {
               RESIZE_DISPLAY_EVENT ev;
               ev.old_w = old_width;
               ev.old_h = old_height;
               ev.new_w = wnd_width;
               ev.new_h = wnd_height;
               ev.is_maximized = (wparam == SIZE_MAXIMIZED) ? 1: 0;
               ev.is_restored = (wparam == SIZE_RESTORED) ? 1: 0;

               (*user_resize_proc)(&ev);
            }
         }
         break;
      }

      case WM_SIZING: {
         LPRECT rc = (LPRECT)lparam;
         int w = (rc->right - rc->left);
         int h = (rc->bottom - rc->top);
         int dw = (w % 16);
         int dh = (h % 16);

         switch (wparam) {

            case WMSZ_LEFT:
            case WMSZ_TOPLEFT:
            case WMSZ_BOTTOMLEFT: {
               if (w < 192)
                 rc->left = rc->right - 192;
               else
                 rc->left += dw;
               break;
            }

            case WMSZ_RIGHT:
            case WMSZ_TOPRIGHT:
            case WMSZ_BOTTOMRIGHT: {
               if (w < 192)
                 rc->right = rc->left + 192;
               else
                 rc->right -= dw;
               break;
            }

            case WMSZ_TOP:
            case WMSZ_BOTTOM:
               /* Ignore */
               break;
         }

         switch (wparam) {

            case WMSZ_TOP:
            case WMSZ_TOPLEFT:
            case WMSZ_TOPRIGHT:
               if (h < 96)
                 rc->top = rc->bottom - 96;
               else
                 rc->top += dh;
               break;

            case WMSZ_BOTTOM:
            case WMSZ_BOTTOMLEFT:
            case WMSZ_BOTTOMRIGHT:
               if (h < 96)
                 rc->bottom = rc->top + 96;
               else
                 rc->bottom -= dh;
               break;

            case WMSZ_LEFT:
            case WMSZ_RIGHT:
               /* Ignore */
               break;
         }

         sizing = TRUE;
         return TRUE;
      }

      case WM_PAINT:
         if (win_gfx_driver) {
            PAINTSTRUCT ps;
            BeginPaint(wnd, &ps);
            if (win_gfx_driver && win_gfx_driver->paint)
                win_gfx_driver->paint(&ps.rcPaint);
            EndPaint(wnd, &ps);
            return 0;
         }
         break;

      case WM_SYSCOMMAND:
         if (wparam == SC_MONITORPOWER || wparam == SC_SCREENSAVE) {
            if (_screensaver_policy == ALWAYS_DISABLED
                || (_screensaver_policy == FULLSCREEN_DISABLED
                    && gfx_driver && !gfx_driver->windowed))
            return 0;
         }
         break;

      case WM_INITMENUPOPUP:
         wnd_sysmenu = TRUE;

         if (win_gfx_driver && win_gfx_driver->enter_sysmode)
            win_gfx_driver->enter_sysmode();
         break;

      case WM_MENUSELECT:
         if ((HIWORD(wparam) == 0xFFFF) && (!lparam)) {
            wnd_sysmenu = FALSE;

            if (win_gfx_driver && win_gfx_driver->exit_sysmode)
               win_gfx_driver->exit_sysmode();
         }
         break;

      case WM_MENUCHAR :
         return (MNC_CLOSE<<16)|(wparam&0xffff);

      case WM_CLOSE:
         if (user_close_proc)
            (*user_close_proc)();
         return 0;

      case WM_LBUTTONDOWN:
      case WM_LBUTTONUP:
         if (win_gfx_driver && _mouse_installed) {
            int mx = GET_X_LPARAM(lparam);
            int my = GET_Y_LPARAM(lparam);
            BOOL down = (message == WM_LBUTTONDOWN);
            _al_win_mouse_handle_button(wnd, 1, down, mx, my, TRUE);
         }
         break;

      case WM_MBUTTONDOWN:
      case WM_MBUTTONUP:
         if (win_gfx_driver && _mouse_installed) {
            int mx = GET_X_LPARAM(lparam);
            int my = GET_Y_LPARAM(lparam);
            BOOL down = (message == WM_MBUTTONDOWN);
            _al_win_mouse_handle_button(wnd, 3, down, mx, my, TRUE);
         }
         break;

      case WM_RBUTTONDOWN:
      case WM_RBUTTONUP:
         if (win_gfx_driver && _mouse_installed) {
            int mx = GET_X_LPARAM(lparam);
            int my = GET_Y_LPARAM(lparam);
            BOOL down = (message == WM_RBUTTONDOWN);
            _al_win_mouse_handle_button(wnd, 2, down, mx, my, TRUE);
         }
         break;

#if 0 /* We don't support extra mouse buttons */
      case WM_XBUTTONDOWN:
      case WM_XBUTTONUP:
         if (win_gfx_driver && _mouse_installed) {
            int mx = GET_X_LPARAM(lparam);
            int my = GET_Y_LPARAM(lparam);
            int button = HIWORD(wparam);
            BOOL down = (message == WM_XBUTTONDOWN);
            if (button == XBUTTON1)
               _al_win_mouse_handle_button(wnd, 4, down, mx, my, TRUE);
            else if (button == XBUTTON2)
               _al_win_mouse_handle_button(wnd, 5, down, mx, my, TRUE);
            return TRUE;
         }
         break;
#endif

      case WM_MOUSEWHEEL:
         if (win_gfx_driver && _mouse_installed) {
            int d = GET_WHEEL_DELTA_WPARAM(wparam);
            _al_win_mouse_handle_wheel(wnd, d / WHEEL_DELTA, FALSE);
            return TRUE;
         }
         break;

      case WM_MOUSEMOVE:
         if (win_gfx_driver && _mouse_installed) {
            POINTS p = MAKEPOINTS(lparam);
            _al_win_mouse_handle_move(wnd, p.x, p.y);
            return 0;
         }
         break;

      case WM_SYSKEYDOWN:
         if (_keyboard_installed) {
            int vcode = wparam;
            BOOL repeated  = (lparam >> 30) & 0x1;
            _al_win_kbd_handle_key_press(0, vcode, repeated);
            return 0;
         }
         break;

      case WM_KEYDOWN:
         if (_keyboard_installed) {
            int vcode = wparam;
            int scode = (lparam >> 16) & 0xff;
            BOOL repeated  = (lparam >> 30) & 0x1;
            /* We can't use TranslateMessage() because we don't know if it will
               produce a WM_CHAR or not. */
            _al_win_kbd_handle_key_press(scode, vcode, repeated);
            return 0;
         }
         break;

      case WM_SYSKEYUP:
      case WM_KEYUP:
         if (_keyboard_installed) {
            int vcode = wparam;
            _al_win_kbd_handle_key_release(vcode);
            return 0;
         }
         break;

   }

   /* pass message to default window proc */
   return DefWindowProc(wnd, message, wparam, lparam);
}



/* create_directx_window:
 *  Creates the Allegro window.
 */
static HWND create_directx_window(void)
{
   static int first = 1;
   WNDCLASS wnd_class;
   char fname[1024];
   HWND wnd;

   if (first) {
      /* setup the window class */
      wnd_class.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
      wnd_class.lpfnWndProc = directx_wnd_proc;
      wnd_class.cbClsExtra = 0;
      wnd_class.cbWndExtra = 0;
      wnd_class.hInstance = allegro_inst;
      wnd_class.hIcon = LoadIcon(allegro_inst, L"allegro_icon");
      if (!wnd_class.hIcon)
         wnd_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
      wnd_class.hCursor = LoadCursor(NULL, IDC_ARROW);
      wnd_class.hbrBackground = NULL;
      wnd_class.lpszMenuName = NULL;
      wnd_class.lpszClassName = ALLEGRO_WND_CLASS;

      RegisterClass(&wnd_class);

      /* what are we called? */
      get_executable_name(fname, sizeof(fname));
      ustrlwr(fname);

      usetc(get_extension(fname), 0);
      if (ugetat(fname, -1) == '.')
         usetat(fname, -1, 0);

      do_uconvert(get_filename(fname), U_CURRENT, (char*)wnd_title, U_UNICODE, WND_TITLE_SIZE);

      first = 0;
   }

   /* create the window now */
   wnd = CreateWindowEx(WS_EX_APPWINDOW, ALLEGRO_WND_CLASS, wnd_title,
                        WS_OVERLAPPEDWINDOW,
                        -100, -100, 0, 0,
                        NULL, NULL, allegro_inst, NULL);
   if (!wnd) {
      _TRACE(PREFIX_E "CreateWindowEx() failed (%s)\n", win_err_str(GetLastError()));
      return NULL;
   }

   ShowWindow(wnd, SW_SHOWNORMAL);
   SetForegroundWindow(wnd);
   UpdateWindow(wnd);

   return wnd;
}



/* wnd_thread_proc:
 *  Thread function that handles the messages of the directx window.
 */
static DWORD WINAPI wnd_thread_proc(HANDLE setup_event)
{
   MSG msg;

   _win_thread_init();
   _TRACE(PREFIX_I "window thread starts\n");

   /* setup window */
   allegro_wnd = create_directx_window();
   if (!allegro_wnd)
      goto End;

   /* now the thread it running successfully, let's acknowledge */
   SetEvent(setup_event);

   /* message loop */
   while (GetMessage(&msg, NULL, 0, 0))
     DispatchMessage(&msg);

 End:
   _TRACE(PREFIX_I "window thread exits\n");
   _win_thread_exit();

   return 0;
}



/* init_directx_window:
 *  A thread is created to own the new window.
 */
int init_directx_window(void)
{
   HANDLE events[2];
   long result;
   DWORD threadId;

   /* setup globals */
   msg_call_proc = RegisterWindowMessage(L"Allegro call proc");
   msg_suicide = RegisterWindowMessage(L"Allegro window suicide");

   /* create window thread */
   events[0] = CreateEvent(NULL, FALSE, FALSE, NULL);        /* acknowledges that thread is up */
   events[1] = CreateThread(NULL, 0, wnd_thread_proc, events[0], CREATE_SUSPENDED, &threadId);
   ResumeThread(events[1]);

   result = WaitForMultipleObjects(2, events, FALSE, INFINITE);

   CloseHandle(events[0]);

   switch (result) {
   case WAIT_OBJECT_0:    /* window was created successfully */
      wnd_thread = events[1];
      SetThreadPriority(wnd_thread, THREAD_PRIORITY_ABOVE_NORMAL);
      break;
   default:               /* thread failed to create window */
      return -1;
   }

   /* initialize gfx critical section */
   InitializeCriticalSection(&gfx_crit_sect);

   /* save window style */
   old_style = GetWindowLong(allegro_wnd, GWL_STYLE);

   return 0;
}



/* exit_directx_window:
 *  If a user window was hooked, the old window proc is set. Otherwise
 *  the window thread is destroyed.
 */
void exit_directx_window(void)
{
   /* Destroy the window: we cannot directly use DestroyWindow() because
    * we are not running in the same thread as that of the window.
    */
   PostMessage(allegro_wnd, msg_suicide, 0, 0);

   /* wait until the window thread ends */
   WaitForSingleObject(wnd_thread, INFINITE);
   CloseHandle(wnd_thread);
   wnd_thread = NULL;

   DeleteCriticalSection(&gfx_crit_sect);
}



/* restore_window_style:
 *  Restores the previous style of the window.
 */
void restore_window_style(void)
{
   SetWindowLong(allegro_wnd, GWL_STYLE, old_style);
   SetWindowPos(allegro_wnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}



/* adjust_window:
 *  Moves and resizes the window if we have full control over it.
 */
int adjust_window(int w, int h)
{
   RECT working_area, win_size;
   TITLEBARINFO tb_info;
   HMODULE user32_handle;
   typedef BOOL (WINAPI *func)(HWND, PTITLEBARINFO);
   func get_title_bar_info = NULL;
   int tb_height = 0;
   static int last_w=-1, last_h=-1;

   SystemParametersInfo(SPI_GETWORKAREA, 0, &working_area, 0);

   if ((last_w == -1) && (last_h == -1)) {
      /* first window placement: try to center it */
      last_wnd_x = (working_area.left + working_area.right - w)/2;
      last_wnd_y = (working_area.top + working_area.bottom - h)/2;
   }
   else {
      /* try to get the height of the window's title bar */
      user32_handle = GetModuleHandle(L"user32");
      if (user32_handle) {
         get_title_bar_info
            = (func)GetProcAddress(user32_handle, "GetTitleBarInfo");
         if (get_title_bar_info) {
            tb_info.cbSize = sizeof(TITLEBARINFO);
            if (get_title_bar_info(allegro_wnd, &tb_info))
               tb_height
                  = tb_info.rcTitleBar.bottom - tb_info.rcTitleBar.top;
         }
      }
      if (!user32_handle || !get_title_bar_info)
         tb_height = GetSystemMetrics(SM_CYSIZE);

      /* try to center the window relative to its last position */
      last_wnd_x += (last_w - w)/2;
      last_wnd_y += (last_h - h)/2;

      if (last_wnd_x + w >= working_area.right)
         last_wnd_x = working_area.right - w;
      if (last_wnd_y + h >= working_area.bottom)
         last_wnd_y = working_area.bottom - h;
      if (last_wnd_x < working_area.left)
         last_wnd_x = working_area.left;
      if (last_wnd_y - tb_height < working_area.top)
         last_wnd_y = working_area.top + tb_height;
   }

#ifdef ALLEGRO_COLORCONV_ALIGNED_WIDTH
   last_wnd_x &= 0xfffffffc;
#endif

   win_size.left = last_wnd_x;
   win_size.top = last_wnd_y;
   win_size.right = last_wnd_x+w;
   win_size.bottom = last_wnd_y+h;

   last_w = w;
   last_h = h;

   /* retrieve the size of the decorated window */
   AdjustWindowRect(&win_size, GetWindowLong(allegro_wnd, GWL_STYLE), FALSE);

   /* display the window */
   MoveWindow(allegro_wnd, win_size.left, win_size.top,
              win_size.right - win_size.left, win_size.bottom - win_size.top, TRUE);

   /* check that the actual window size is the one requested */
   GetClientRect(allegro_wnd, &win_size);
   if (((win_size.right - win_size.left) != w) || ((win_size.bottom - win_size.top) != h))
      return -1;

   wnd_x = last_wnd_x;
   wnd_y = last_wnd_y;
   wnd_width = w;
   wnd_height = h;

   return 0;
}



/* save_window_pos:
 *  Stores the position of the current window before closing it so that
 *  it can be used as the initial position for the next window.
 */
void save_window_pos(void)
{
   last_wnd_x = wnd_x;
   last_wnd_y = wnd_y;
}



/* win_get_window:
 *  Returns the Allegro window handle.
 */
HWND win_get_window(void)
{
   return allegro_wnd;
}
