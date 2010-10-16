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
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"

#ifndef SCAN_DEPEND
   #include <string.h>
   #include <process.h>
   #include <time.h>
#endif

#ifndef ALLEGRO_WINDOWS
   #error something is wrong with the makefile
#endif

#define PREFIX_I                "al-wwnd INFO: "
#define PREFIX_W                "al-wwnd WARNING: "
#define PREFIX_E                "al-wwnd ERROR: "


/* general */
static HWND allegro_wnd = NULL;
char wnd_title[WND_TITLE_SIZE];  /* ASCII string */
int wnd_x = 0;
int wnd_y = 0;
int wnd_width = 0;
int wnd_height = 0;
int wnd_sysmenu = FALSE;

static int last_wnd_x = -1;
static int last_wnd_y = -1;

static int window_is_initialized = FALSE;

/* graphics */
WIN_GFX_DRIVER *win_gfx_driver;
CRITICAL_SECTION gfx_crit_sect;
int gfx_crit_sect_nesting = 0;

/* close button user hook */
void (*user_close_proc)(void) = NULL;

/* window thread internals */
#define ALLEGRO_WND_CLASS "AllegroWindow"
static HWND user_wnd = NULL;
static WNDPROC user_wnd_proc = NULL;
static HANDLE wnd_thread = NULL;
static HWND (*wnd_create_proc)(WNDPROC) = NULL;
static int old_style = 0;

static int (*wnd_msg_pre_proc)(HWND, UINT, WPARAM, LPARAM, int *) = NULL;

/* custom window msgs */
#define SWITCH_TIMER  1
static UINT msg_call_proc = 0;
static UINT msg_suicide = 0;

/* window modules management */
struct WINDOW_MODULES {
   int keyboard;
   int mouse;
   int joystick;
   int joy_type;
   int sound;
   int digi_card;
   int midi_card;
   int sound_input;
   int digi_input_card;
   int midi_input_card;
};

/* Used in adjust_window(). */
#ifndef CCHILDREN_TITLEBAR
   #define CCHILDREN_TITLEBAR 5
typedef struct {
   DWORD cbSize;
   RECT  rcTitleBar;
   DWORD rgstate[CCHILDREN_TITLEBAR + 1];
} TITLEBARINFO, *PTITLEBARINFO, *LPTITLEBARINFO;
#endif /* ifndef CCHILDREN_TITLEBAR */



/* init_window_modules:
 *  Initialises the modules that are specified by the WM argument.
 */
static int init_window_modules(struct WINDOW_MODULES *wm)
{
   if (wm->keyboard)
      install_keyboard();

   if (wm->mouse)
      install_mouse();

   if (wm->joystick)
      install_joystick(wm->joy_type);

   if (wm->sound)
      install_sound(wm->digi_card, wm->midi_card, NULL);

   if (wm->sound_input)
      install_sound_input(wm->digi_input_card, wm->midi_input_card);

   return 0;
}



/* exit_window_modules:
 *  Removes the modules that depend upon the main window:
 *   - keyboard (DirectInput),
 *   - mouse (DirectInput),
 *   - joystick (DirectInput),
 *   - sound (DirectSound),
 *   - sound input (DirectSoundCapture).
 *  If WM is not NULL, record which modules are really removed.
 */
static void exit_window_modules(struct WINDOW_MODULES *wm)
{
   if (wm)
      memset(wm, 0, sizeof(*wm));

   if (_keyboard_installed) {
     if (wm)
         wm->keyboard = TRUE;

      remove_keyboard();
   }

   if (_mouse_installed) {
      if (wm)
         wm->mouse = TRUE;

      remove_mouse();
   }

   if (_joystick_installed) {
      if (wm) {
         wm->joystick = TRUE;
         wm->joy_type = _joy_type;
      }

      remove_joystick();
   }

   if (_sound_installed) {
      if (wm) {
         wm->sound = TRUE;
         wm->digi_card = digi_card;
         wm->midi_card = midi_card;
      }

      remove_sound();
   }

   if (_sound_input_installed) {
      if (wm) {
         wm->sound_input = TRUE;
         wm->digi_input_card = digi_input_card;
         wm->midi_input_card = midi_input_card;
      }

      remove_sound_input();
   }
}



/* wnd_call_proc:
 *  Instructs the window thread to call the specified procedure and
 *  waits until the procedure has returned.
 */
int wnd_call_proc(int (*proc) (void))
{
   return SendMessage(allegro_wnd, msg_call_proc, (DWORD) proc, 0);
}



/* wnd_schedule_proc:
 *  Instructs the window thread to call the specified procedure but
 *  doesn't wait until the procedure has returned.
 */
void wnd_schedule_proc(int (*proc) (void))
{
   PostMessage(allegro_wnd, msg_call_proc, (DWORD) proc, 0);
}



/* directx_wnd_proc:
 *  Window procedure for the Allegro window class.
 */
static LRESULT CALLBACK directx_wnd_proc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam)
{
   PAINTSTRUCT ps;

   if (message == msg_call_proc)
      return ((int (*)(void))wparam) ();

   if (message == msg_suicide) {
      DestroyWindow(wnd);
      return 0;
   }

   /* Call user callback if available */
   if (wnd_msg_pre_proc){
      int retval = 0;
      if (wnd_msg_pre_proc(wnd, message, wparam, lparam, &retval) == 0)
         return retval;
   }

   /* See get_reverse_mapping() in wkeybd.c to see what this is for. */
   if (FALSE && (message == WM_KEYDOWN || message == WM_SYSKEYDOWN)) {
      static char name[256];
      TCHAR str[256];
      WCHAR wstr[256];

      GetKeyNameText(lparam, str, sizeof str);
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str, -1, wstr, sizeof wstr);
      uconvert((char *)wstr, U_UNICODE, name, U_CURRENT, sizeof name);
      _TRACE(PREFIX_I" key[%s] = 0x%08lx;\n", name, lparam & 0x1ff0000);
   }

   switch (message) {

      case WM_CREATE:
         if (!user_wnd_proc)
            allegro_wnd = wnd;
         break;

      case WM_DESTROY:
         if (user_wnd_proc) {
            exit_window_modules(NULL);
            _win_reset_switch_mode();
         }
         else {
            PostQuitMessage(0);
         }

         allegro_wnd = NULL;
         break;

      case WM_SETCURSOR:
         if (!user_wnd_proc || _mouse_installed) {
            mouse_set_syscursor();
            return 1;  /* not TRUE */
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

            if (gfx_driver && !gfx_driver->windowed) {
               /* 1.2s delay to let Windows complete the switch in fullscreen mode */
               SetTimer(allegro_wnd, SWITCH_TIMER, 1200, NULL);
            }
            else {
               /* no delay in windowed mode */
               _win_switch_in();
            }
         }
         break;

      case WM_TIMER:
         if (wparam == SWITCH_TIMER) {
            KillTimer(allegro_wnd, SWITCH_TIMER);
            _win_switch_in();
            return 0;
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

      case WM_SIZE:
         wnd_width = LOWORD(lparam);
         wnd_height = HIWORD(lparam);
         break;

      case WM_PAINT:
         if (!user_wnd_proc || win_gfx_driver) {
            BeginPaint(wnd, &ps);
            if (win_gfx_driver && win_gfx_driver->paint)
                win_gfx_driver->paint(&ps.rcPaint);
            EndPaint(wnd, &ps);
            return 0;
         }
         break;

      case WM_KEYDOWN:
      case WM_KEYUP:
      case WM_SYSKEYDOWN:
      case WM_SYSKEYUP:
         /* Disable the default message-based key handler
          * in order to prevent conflicts on NT kernels.
          */
         if (!user_wnd_proc || _keyboard_installed)
            return 0;
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
         mouse_set_sysmenu(TRUE);

         if (win_gfx_driver && win_gfx_driver->enter_sysmode)
            win_gfx_driver->enter_sysmode();
         break;

      case WM_MENUSELECT:
         if ((HIWORD(wparam) == 0xFFFF) && (!lparam)) {
            wnd_sysmenu = FALSE;
            mouse_set_sysmenu(FALSE);

            if (win_gfx_driver && win_gfx_driver->exit_sysmode)
               win_gfx_driver->exit_sysmode();
         }
         break;

      case WM_MENUCHAR :
         return (MNC_CLOSE<<16)|(wparam&0xffff);
         
      case WM_CLOSE:
         if (!user_wnd_proc) {
            if (user_close_proc)
               (*user_close_proc)();
            return 0;
         }
         break;
   }

   /* pass message to default window proc */
   if (user_wnd_proc)
      return CallWindowProc(user_wnd_proc, wnd, message, wparam, lparam);
   else
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
      wnd_class.style = CS_HREDRAW | CS_VREDRAW;
      wnd_class.lpfnWndProc = directx_wnd_proc;
      wnd_class.cbClsExtra = 0;
      wnd_class.cbWndExtra = 0;
      wnd_class.hInstance = allegro_inst;
      wnd_class.hIcon = LoadIcon(allegro_inst, "allegro_icon");
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

      do_uconvert(get_filename(fname), U_CURRENT, wnd_title, U_ASCII, WND_TITLE_SIZE);

      first = 0;
   }

   /* create the window now */
   wnd = CreateWindowEx(WS_EX_APPWINDOW, ALLEGRO_WND_CLASS, wnd_title,
                        WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX,
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
static void wnd_thread_proc(HANDLE setup_event)
{
   int result;
   MSG msg;

   _win_thread_init();
   _TRACE(PREFIX_I "window thread starts\n");

   /* setup window */
   if (wnd_create_proc)
      allegro_wnd = wnd_create_proc(directx_wnd_proc);
   else
      allegro_wnd = create_directx_window();

   if (!allegro_wnd)
      goto End;

   /* now the thread it running successfully, let's acknowledge */
   SetEvent(setup_event);

   /* message loop */
   while (TRUE) {
      result = MsgWaitForMultipleObjects(_win_input_events, _win_input_event_id, FALSE, INFINITE, QS_ALLINPUT);
      if ((result >= WAIT_OBJECT_0) && (result < WAIT_OBJECT_0 + _win_input_events)) {
         /* one of the registered events is in signaled state */
         (*_win_input_event_handler[result - WAIT_OBJECT_0])();
      }
      else if (result == WAIT_OBJECT_0 + _win_input_events) {
         /* messages are waiting in the queue */
         while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
            if (GetMessage(&msg, NULL, 0, 0)) {
               DispatchMessage(&msg);
            }
            else {
               goto End;
            }
         }
      }
   }

 End:
   _TRACE(PREFIX_I "window thread exits\n");
   _win_thread_exit();
}



/* init_directx_window:
 *  If the user called win_set_window, the user window will be hooked to receive
 *  messages from Allegro. Otherwise a thread is created to own the new window.
 */
int init_directx_window(void)
{
   union {
     POINT p;
     RECT r;
   } win_rect;
   HANDLE events[2];
   long result;

   /* setup globals */
   msg_call_proc = RegisterWindowMessage("Allegro call proc");
   msg_suicide = RegisterWindowMessage("Allegro window suicide");

   if (user_wnd) {
      /* initializes input module and requests dedicated thread */
      _win_input_init(TRUE);

      /* hook the user window */
      user_wnd_proc = (WNDPROC) SetWindowLong(user_wnd, GWL_WNDPROC, (long)directx_wnd_proc);
      if (!user_wnd_proc)
         return -1;

      allegro_wnd = user_wnd;

      /* retrieve the window dimensions */
      GetWindowRect(allegro_wnd, &win_rect.r);
      ClientToScreen(allegro_wnd, &win_rect.p);
      ClientToScreen(allegro_wnd, &win_rect.p + 1);
      wnd_x = win_rect.r.left;
      wnd_y = win_rect.r.top;
      wnd_width = win_rect.r.right - win_rect.r.left;
      wnd_height = win_rect.r.bottom - win_rect.r.top;
   }
   else {
      /* initializes input module without dedicated thread */
      _win_input_init(FALSE);

      /* create window thread */
      events[0] = CreateEvent(NULL, FALSE, FALSE, NULL);        /* acknowledges that thread is up */
      events[1] = (HANDLE) _beginthread(wnd_thread_proc, 0, events[0]);
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
   if (user_wnd) {
      /* restore old window proc */
      SetWindowLong(user_wnd, GWL_WNDPROC, (long)user_wnd_proc);
      user_wnd_proc = NULL;
      user_wnd = NULL;
      allegro_wnd = NULL;
   }
   else {
      /* Destroy the window: we cannot directly use DestroyWindow() because
       * we are not running in the same thread as that of the window.
       */
      PostMessage(allegro_wnd, msg_suicide, 0, 0);

      /* wait until the window thread ends */
      WaitForSingleObject(wnd_thread, INFINITE);
      wnd_thread = NULL;
   }

   DeleteCriticalSection(&gfx_crit_sect);

   _win_input_exit();
   
   window_is_initialized = FALSE;
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

   if (!user_wnd) {
      SystemParametersInfo(SPI_GETWORKAREA, 0, &working_area, 0);

      if ((last_w == -1) && (last_h == -1)) {
         /* first window placement: try to center it */
         last_wnd_x = (working_area.left + working_area.right - w)/2;
         last_wnd_y = (working_area.top + working_area.bottom - h)/2;
      }
      else {
         /* try to get the height of the window's title bar */
         user32_handle = GetModuleHandle("user32");
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
   }

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



/* win_set_window:
 *  Selects an user-defined window for Allegro or
 *  the built-in window if NULL is passed.
 */
void win_set_window(HWND wnd)
{
   static int (*saved_scbc)(void (*proc)(void)) = NULL;
   struct WINDOW_MODULES wm;

   if (window_is_initialized || !wnd) {
      exit_window_modules(&wm);
      exit_directx_window();
   }

   user_wnd = wnd;

   /* The user retains full control over the close button if he registers
      a user-defined window. */
   if (user_wnd) {
      if (!saved_scbc)
         saved_scbc = system_directx.set_close_button_callback;

      system_directx.set_close_button_callback = NULL;
   }
   else {
      if (saved_scbc)
         system_directx.set_close_button_callback = saved_scbc;
   }

   if (window_is_initialized || !wnd) {
      init_directx_window();
      init_window_modules(&wm);
   }
   
   window_is_initialized = TRUE;
}



/* win_get_window:
 *  Returns the Allegro window handle.
 */
HWND win_get_window(void)
{
   return allegro_wnd;
}


void win_set_msg_pre_proc(int (*proc)(HWND, UINT, WPARAM, LPARAM, int *))
{
   wnd_msg_pre_proc = proc;
}


/* win_set_wnd_create_proc:
 *  Sets a custom window creation proc.
 */
void win_set_wnd_create_proc(HWND (*proc)(WNDPROC))
{
   wnd_create_proc = proc;
}



/* win_grab_input:
 *  Grabs the input devices.
 */
void win_grab_input(void)
{
   wnd_schedule_proc(key_dinput_acquire);
   wnd_schedule_proc(mouse_dinput_grab);
   wnd_schedule_proc(joystick_dinput_acquire);
}
