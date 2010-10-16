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
 *      Windows mouse driver.
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */


#define DIRECTINPUT_VERSION 0x0300

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"

#ifndef SCAN_DEPEND
   #include <process.h>
   #include <dinput.h>
#endif

#ifndef ALLEGRO_WINDOWS
   #error something is wrong with the makefile
#endif

#define PREFIX_I                "al-wmouse INFO: "
#define PREFIX_W                "al-wmouse WARNING: "
#define PREFIX_E                "al-wmouse ERROR: "


static MOUSE_DRIVER mouse_directx;


_DRIVER_INFO _mouse_driver_list[] =
{
   {MOUSE_DIRECTX, &mouse_directx, TRUE},
   {MOUSEDRV_NONE, &mousedrv_none, TRUE},
   {0, NULL, 0}
};


static int mouse_directx_init(void);
static void mouse_directx_exit(void);
static void mouse_directx_position(int x, int y);
static void mouse_directx_set_range(int x1, int y1, int x2, int y2);
static void mouse_directx_set_speed(int xspeed, int yspeed);
static void mouse_directx_get_mickeys(int *mickeyx, int *mickeyy);
static void mouse_directx_enable_hardware_cursor(int mode);
static int mouse_directx_select_system_cursor(int cursor);

static MOUSE_DRIVER mouse_directx =
{
   MOUSE_DIRECTX,
   empty_string,
   empty_string,
   "DirectInput mouse",
   mouse_directx_init,
   mouse_directx_exit,
   NULL,                       // AL_METHOD(void, poll, (void));
   NULL,                       // AL_METHOD(void, timer_poll, (void));
   mouse_directx_position,
   mouse_directx_set_range,
   mouse_directx_set_speed,
   mouse_directx_get_mickeys,
   NULL,                       // AL_METHOD(int, analyse_data, (AL_CONST char *buffer, int size));
   mouse_directx_enable_hardware_cursor,
   mouse_directx_select_system_cursor
};


HCURSOR _win_hcursor = NULL;	/* Hardware cursor to display */


#define DINPUT_BUFFERSIZE 256
static HANDLE mouse_input_event = NULL;
static LPDIRECTINPUT mouse_dinput = NULL;
static LPDIRECTINPUTDEVICE mouse_dinput_device = NULL;

static int dinput_buttons = 0;
static int dinput_wheel = FALSE;

static int mouse_swap_button = FALSE;     /* TRUE means buttons 1 and 2 are swapped */

static int dinput_x = 0;              /* tracked dinput positon */
static int dinput_y = 0;

static int mouse_mx = 0;              /* internal position, in mickeys */
static int mouse_my = 0;

static int mouse_sx = 2;              /* mickey -> pixel scaling factor */
static int mouse_sy = 2;

#define MAF_DEFAULT 1                 /* mouse acceleration parameters */
static int mouse_accel_fact = MAF_DEFAULT;
static int mouse_accel_mult = MAF_DEFAULT;
static int mouse_accel_thr1 = 5 * 5;
static int mouse_accel_thr2 = 16 * 16;

static int mouse_minx = 0;            /* mouse range */
static int mouse_miny = 0;
static int mouse_maxx = 319;
static int mouse_maxy = 199;

static int mymickey_x = 0;            /* for get_mouse_mickeys() */
static int mymickey_y = 0;
static int mymickey_ox = 0;
static int mymickey_oy = 0;

#define MICKEY_TO_COORD_X(n)        ((n) / mouse_sx)
#define MICKEY_TO_COORD_Y(n)        ((n) / mouse_sy)

#define COORD_TO_MICKEY_X(n)        ((n) * mouse_sx)
#define COORD_TO_MICKEY_Y(n)        ((n) * mouse_sy)

#define CLEAR_MICKEYS()                       \
{                                             \
   if (gfx_driver && gfx_driver->windowed) {  \
      POINT p;                                \
                                              \
      GetCursorPos(&p);                       \
                                              \
      p.x -= wnd_x;                           \
      p.y -= wnd_y;                           \
                                              \
      mymickey_ox = p.x;                      \
      mymickey_oy = p.y;                      \
   }                                          \
   else {                                     \
      dinput_x = 0;                           \
      dinput_y = 0;                           \
      mymickey_ox = 0;                        \
      mymickey_oy = 0;                        \
   }                                          \
}

#define READ_CURSOR_POS(p)                          \
{                                                   \
   GetCursorPos(&p);                                \
                                                    \
   p.x -= wnd_x;                                    \
   p.y -= wnd_y;                                    \
                                                    \
   if ((p.x < mouse_minx) || (p.x > mouse_maxx) ||  \
       (p.y < mouse_miny) || (p.y > mouse_maxy)) {  \
      if (_mouse_on) {                              \
         _mouse_on = FALSE;                         \
         wnd_schedule_proc(mouse_set_syscursor);    \
      }                                             \
   }                                                \
   else {                                           \
      if (!_mouse_on) {                             \
         _mouse_on = TRUE;                          \
         wnd_schedule_proc(mouse_set_syscursor);    \
      }                                             \
      _mouse_x = p.x;                               \
      _mouse_y = p.y;                               \
   }                                                \
}



/* dinput_err_str:
 *  Returns a DirectInput error string.
 */
#ifdef DEBUGMODE
static char* dinput_err_str(long err)
{
   static char err_str[64];

   switch (err) {

      case DIERR_ACQUIRED:
         _al_sane_strncpy(err_str, "the device is acquired", sizeof(err_str));
         break;

      case DIERR_NOTACQUIRED:
         _al_sane_strncpy(err_str, "the device is not acquired", sizeof(err_str));
         break;

      case DIERR_INPUTLOST:
         _al_sane_strncpy(err_str, "access to the device was not granted", sizeof(err_str));
         break;

      case DIERR_INVALIDPARAM:
         _al_sane_strncpy(err_str, "the device does not have a selected data format", sizeof(err_str));
         break;

#ifdef DIERR_OTHERAPPHASPRIO
      /* this is not a legacy DirectX 3 error code */
      case DIERR_OTHERAPPHASPRIO:
         _al_sane_strncpy(err_str, "can't acquire the device in background", sizeof(err_str));
         break;
#endif

      default:
         _al_sane_strncpy(err_str, "unknown error", sizeof(err_str));
   }

   return err_str;
}
#else
#define dinput_err_str(hr) "\0"
#endif



/* mouse_dinput_handle_event: [input thread]
 *  Handles a single event.
 */
static void mouse_dinput_handle_event(int ofs, int data)
{
   static int last_data_x = 0;
   static int last_data_y = 0;
   static int last_was_x = 0;
   int mag;

   switch (ofs) {

      case DIMOFS_X:
         if (!gfx_driver || !gfx_driver->windowed) {
            if (last_was_x)
               last_data_y = 0;
            last_data_x = data;
            last_was_x = 1;
            if (mouse_accel_mult) {
               mag = last_data_x*last_data_x + last_data_y*last_data_y;
               if (mag >= mouse_accel_thr2)
                  data *= (mouse_accel_mult<<1);
               else if (mag >= mouse_accel_thr1) 
                  data *= mouse_accel_mult;
            }

            dinput_x += data;
         }
         break;

      case DIMOFS_Y:
         if (!gfx_driver || !gfx_driver->windowed) {
            if (!last_was_x)
               last_data_x = 0;
            last_data_y = data;
            last_was_x = 0;
            if (mouse_accel_mult) {
               mag = last_data_x*last_data_x + last_data_y*last_data_y;
               if (mag >= mouse_accel_thr2)
                  data *= (mouse_accel_mult<<1);
               else if (mag >= mouse_accel_thr1) 
                  data *= mouse_accel_mult;
            }

            dinput_y += data;
         }
         break;

      case DIMOFS_Z:
         if (dinput_wheel && _mouse_on)
            _mouse_z += data/120;
         break;

      case DIMOFS_BUTTON0:
         if (data & 0x80) {
            if (_mouse_on)
               _mouse_b |= (mouse_swap_button ? 2 : 1);
         }
         else
            _mouse_b &= ~(mouse_swap_button ? 2 : 1);
         break;

      case DIMOFS_BUTTON1:
         if (data & 0x80) {
            if (_mouse_on)
               _mouse_b |= (mouse_swap_button ? 1 : 2);
         }
         else
            _mouse_b &= ~(mouse_swap_button ? 1 : 2);
         break;

      case DIMOFS_BUTTON2:
         if (data & 0x80) {
            if (_mouse_on)
               _mouse_b |= 4;
         }
         else
            _mouse_b &= ~4;
         break;

      case DIMOFS_BUTTON3:
         if (data & 0x80) {
            if (_mouse_on)
               _mouse_b |= 8;
         }
         else
            _mouse_b &= ~8;
         break;
   }
}



/* mouse_dinput_handle: [input thread]
 *  Handles queued mouse input.
 */
static void mouse_dinput_handle(void)
{
   static DIDEVICEOBJECTDATA message_buffer[DINPUT_BUFFERSIZE];
   unsigned long int waiting_messages;
   HRESULT hr;
   unsigned long int i;

   /* the whole buffer is free */
   waiting_messages = DINPUT_BUFFERSIZE;

   /* fill the buffer */
   hr = IDirectInputDevice_GetDeviceData(mouse_dinput_device,
                                         sizeof(DIDEVICEOBJECTDATA),
                                         message_buffer,
                                         &waiting_messages,
                                         0);

   /* was device lost ? */
   if ((hr == DIERR_NOTACQUIRED) || (hr == DIERR_INPUTLOST)) {
      /* reacquire device */
      _TRACE(PREFIX_W "mouse device not acquired or lost\n");
      wnd_schedule_proc(mouse_dinput_acquire);
   }
   else if (FAILED(hr)) {  /* other error? */
      _TRACE(PREFIX_E "unexpected error while filling mouse message buffer\n");
   }
   else {
      /* normally only this case should happen */
      for (i = 0; i < waiting_messages; i++) {
         mouse_dinput_handle_event(message_buffer[i].dwOfs,
                                   message_buffer[i].dwData);
      }

      if (gfx_driver && gfx_driver->windowed) {
         /* windowed input mode */
         if (!wnd_sysmenu) {
            POINT p;

            READ_CURSOR_POS(p);

            mymickey_x += p.x - mymickey_ox;
            mymickey_y += p.y - mymickey_oy;

            mymickey_ox = p.x;
            mymickey_oy = p.y;

            _handle_mouse_input();
         }
      }
      else {
         /* fullscreen input mode */
         mymickey_x += dinput_x - mymickey_ox;
         mymickey_y += dinput_y - mymickey_oy;

         mymickey_ox = dinput_x;
         mymickey_oy = dinput_y;

         _mouse_x = MICKEY_TO_COORD_X(mouse_mx + dinput_x);
         _mouse_y = MICKEY_TO_COORD_Y(mouse_my + dinput_y);

         if ((_mouse_x < mouse_minx) || (_mouse_x > mouse_maxx) ||
             (_mouse_y < mouse_miny) || (_mouse_y > mouse_maxy)) {

            _mouse_x = CLAMP(mouse_minx, _mouse_x, mouse_maxx);
            _mouse_y = CLAMP(mouse_miny, _mouse_y, mouse_maxy);

            mouse_mx = COORD_TO_MICKEY_X(_mouse_x);
            mouse_my = COORD_TO_MICKEY_Y(_mouse_y);

            CLEAR_MICKEYS();
         }

         if (!_mouse_on) {
            _mouse_on = TRUE;
            wnd_schedule_proc(mouse_set_syscursor);
         }

         _handle_mouse_input();
      }
   }
}



/* mouse_dinput_acquire: [window thread]
 *  Acquires the mouse device.
 */
int mouse_dinput_acquire(void)
{
   HRESULT hr;

   if (mouse_dinput_device) {
      _mouse_b = 0;

      hr = IDirectInputDevice_Acquire(mouse_dinput_device);

      if (FAILED(hr)) {
	 _TRACE(PREFIX_E "acquire mouse failed: %s\n", dinput_err_str(hr));
	 return -1;
      }

      /* The cursor may not be in the client area
       * so we don't set _mouse_on here.
       */

      return 0;
   }
   else {
      return -1;
   }
}



/* mouse_dinput_unacquire: [window thread]
 *  Unacquires the mouse device.
 */
int mouse_dinput_unacquire(void)
{
   if (mouse_dinput_device) {
      IDirectInputDevice_Unacquire(mouse_dinput_device);

      _mouse_b = 0;
      _mouse_on = FALSE;

      return 0;
   }
   else {
      return -1;
   }
}



/* mouse_dinput_grab: [window thread]
 *  Grabs the mouse device.
 */
int mouse_dinput_grab(void)
{
   HRESULT hr;
   DWORD level;
   HWND allegro_wnd = win_get_window();

   if (mouse_dinput_device) {
      /* necessary in order to set the cooperative level */
      mouse_dinput_unacquire();

      if (gfx_driver && !gfx_driver->windowed) {
         level = DISCL_FOREGROUND | DISCL_EXCLUSIVE;
         _TRACE(PREFIX_I "foreground exclusive cooperative level requested for mouse\n");
      }
      else {
         level = DISCL_FOREGROUND | DISCL_NONEXCLUSIVE;
         _TRACE(PREFIX_I "foreground non-exclusive cooperative level requested for mouse\n");
      }

      /* set cooperative level */
      hr = IDirectInputDevice_SetCooperativeLevel(mouse_dinput_device, allegro_wnd, level);
      if (FAILED(hr)) {
         _TRACE(PREFIX_E "set cooperative level failed: %s\n", dinput_err_str(hr));
         return -1;
      }

      mouse_dinput_acquire();

      /* update the system cursor */
      mouse_set_syscursor();

      return 0;
   }
   else {
      /* update the system cursor */
      mouse_set_syscursor();

      return -1;
   }
}



/* mouse_set_syscursor: [window thread]
 *  Selects whatever system cursor we should display.
 */
int mouse_set_syscursor(void)
{
   HWND allegro_wnd = win_get_window();
   if ((mouse_dinput_device && _mouse_on) || (gfx_driver && !gfx_driver->windowed)) {
      SetCursor(_win_hcursor);
      /* Make sure the cursor is removed by the system. */
      PostMessage(allegro_wnd, WM_MOUSEMOVE, 0, 0);
   }
   else
      SetCursor(LoadCursor(NULL, IDC_ARROW));

   return 0;
}



/* mouse_set_sysmenu: [window thread]
 *  Changes the state of the mouse when going to/from sysmenu mode.
 */
int mouse_set_sysmenu(int state)
{
   POINT p;

   if (mouse_dinput_device) {
      if (state == TRUE) {
         if (_mouse_on) {
            _mouse_on = FALSE;
            mouse_set_syscursor();
         }
      }
      else {
         READ_CURSOR_POS(p);
      }

      _handle_mouse_input();
   }

   return 0;
}



/* mouse_dinput_exit: [primary thread]
 *  Shuts down the DirectInput mouse device.
 */
static int mouse_dinput_exit(void)
{
   if (mouse_dinput_device) {
      /* unregister event handler first */
      _win_input_unregister_event(mouse_input_event);

      /* unacquire device */
      wnd_call_proc(mouse_dinput_unacquire);

      /* now it can be released */
      IDirectInputDevice_Release(mouse_dinput_device);
      mouse_dinput_device = NULL;
   }

   /* release DirectInput interface */
   if (mouse_dinput) {
      IDirectInput_Release(mouse_dinput);
      mouse_dinput = NULL;
   }

   /* close event handle */
   if (mouse_input_event) {
      CloseHandle(mouse_input_event);
      mouse_input_event = NULL;
   }

   /* update the system cursor */
   wnd_schedule_proc(mouse_set_syscursor);

   return 0;
}



/* mouse_enum_callback: [primary thread]
 *  Helper function for finding out how many buttons we have.
 */
static BOOL CALLBACK mouse_enum_callback(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
   if (memcmp(&lpddoi->guidType, &GUID_ZAxis, sizeof(GUID)) == 0)
      dinput_wheel = TRUE;
   else if (memcmp(&lpddoi->guidType, &GUID_Button, sizeof(GUID)) == 0)
      dinput_buttons++;

   return DIENUM_CONTINUE;
}



/* mouse_dinput_init: [primary thread]
 *  Sets up the DirectInput mouse device.
 */
static int mouse_dinput_init(void)
{
   HRESULT hr;
   DIPROPDWORD property_buf_size =
   {
      /* the header */
      {
	 sizeof(DIPROPDWORD),   // diph.dwSize
	 sizeof(DIPROPHEADER),  // diph.dwHeaderSize
	 0,                     // diph.dwObj
	 DIPH_DEVICE,           // diph.dwHow
      },

      /* the data */
      DINPUT_BUFFERSIZE,        // dwData
   };

   /* Get DirectInput interface */
   hr = CoCreateInstance(&CLSID_DirectInput, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectInput, &mouse_dinput);
   if (FAILED(hr))
      goto Error;

   hr = IDirectInput_Initialize(mouse_dinput, allegro_inst, DIRECTINPUT_VERSION);
   if (FAILED(hr))
      goto Error;

   /* Create the mouse device */
   hr = IDirectInput_CreateDevice(mouse_dinput, &GUID_SysMouse, &mouse_dinput_device, NULL);
   if (FAILED(hr))
      goto Error;

   /* Find how many buttons */
   dinput_buttons = 0;
   dinput_wheel = FALSE;

   hr = IDirectInputDevice_EnumObjects(mouse_dinput_device, mouse_enum_callback, NULL, DIDFT_PSHBUTTON | DIDFT_AXIS);
   if (FAILED(hr))
      goto Error;

   /* Check to see if the buttons are swapped (left-hand) */
   mouse_swap_button = GetSystemMetrics(SM_SWAPBUTTON);   

   /* Set data format */
   hr = IDirectInputDevice_SetDataFormat(mouse_dinput_device, &c_dfDIMouse);
   if (FAILED(hr))
      goto Error;

   /* Set buffer size */
   hr = IDirectInputDevice_SetProperty(mouse_dinput_device, DIPROP_BUFFERSIZE, &property_buf_size.diph);
   if (FAILED(hr))
      goto Error;

   /* Enable event notification */
   mouse_input_event = CreateEvent(NULL, FALSE, FALSE, NULL);
   hr = IDirectInputDevice_SetEventNotification(mouse_dinput_device, mouse_input_event);
   if (FAILED(hr))
      goto Error;

   /* Register event handler */
   if (_win_input_register_event(mouse_input_event, mouse_dinput_handle) != 0)
      goto Error;

   /* Grab the device */
   _mouse_on = TRUE;
   wnd_call_proc(mouse_dinput_grab);

   return 0;

 Error:
   mouse_dinput_exit();
   return -1;
}



/* mouse_directx_init: [primary thread]
 */
static int mouse_directx_init(void)
{
   char tmp1[64], tmp2[128];

   /* get user acceleration factor */
   mouse_accel_fact = get_config_int(uconvert_ascii("mouse", tmp1),
                                     uconvert_ascii("mouse_accel_factor", tmp2),
                                     MAF_DEFAULT);

   if (mouse_dinput_init() != 0) {
      /* something has gone wrong */
      _TRACE(PREFIX_E "mouse handler init failed\n");
      return -1;
   }

   /* the mouse handler has successfully set up */
   _TRACE(PREFIX_I "mouse handler starts\n");
   return dinput_buttons;
}



/* mouse_directx_exit: [primary thread]
 */
static void mouse_directx_exit(void)
{
   if (mouse_dinput_device) {
      /* command mouse handler shutdown */
      _TRACE(PREFIX_I "mouse handler exits\n");
      mouse_dinput_exit();
   }
}



/* mouse_directx_position: [primary thread]
 */
static void mouse_directx_position(int x, int y)
{
   _enter_critical();

   _mouse_x = x;
   _mouse_y = y;

   mouse_mx = COORD_TO_MICKEY_X(x);
   mouse_my = COORD_TO_MICKEY_Y(y);

   if (gfx_driver && gfx_driver->windowed)
      SetCursorPos(x+wnd_x, y+wnd_y);

   CLEAR_MICKEYS();

   mymickey_x = mymickey_y = 0;

   /* force mouse update */
   SetEvent(mouse_input_event);

   _exit_critical();
}



/* mouse_directx_set_range: [primary thread]
 */
static void mouse_directx_set_range(int x1, int y1, int x2, int y2)
{
   mouse_minx = x1;
   mouse_miny = y1;
   mouse_maxx = x2;
   mouse_maxy = y2;

   _enter_critical();

   _mouse_x = CLAMP(mouse_minx, _mouse_x, mouse_maxx);
   _mouse_y = CLAMP(mouse_miny, _mouse_y, mouse_maxy);

   mouse_mx = COORD_TO_MICKEY_X(_mouse_x);
   mouse_my = COORD_TO_MICKEY_Y(_mouse_y);

   CLEAR_MICKEYS();

   _exit_critical();

   /* scale up the acceleration multiplier to the range */
   mouse_accel_mult = mouse_accel_fact * MAX(x2-x1+1, y2-y1+1)/320;
}



/* mouse_directx_set_speed: [primary thread]
 */
static void mouse_directx_set_speed(int xspeed, int yspeed)
{
   _enter_critical();

   mouse_sx = MAX(1, xspeed);
   mouse_sy = MAX(1, yspeed);

   mouse_mx = COORD_TO_MICKEY_X(_mouse_x);
   mouse_my = COORD_TO_MICKEY_Y(_mouse_y);

   CLEAR_MICKEYS();

   _exit_critical();
}



/* mouse_directx_get_mickeys: [primary thread]
 */
static void mouse_directx_get_mickeys(int *mickeyx, int *mickeyy)
{
   int temp_x = mymickey_x;
   int temp_y = mymickey_y;

   mymickey_x -= temp_x;
   mymickey_y -= temp_y;

   *mickeyx = temp_x;
   *mickeyy = temp_y;
}



/* mouse_directx_enable_hardware_cursor:
 *  enable the hardware cursor; actually a no-op in Windows, but we need to
 *  put something in the vtable.
 */
static void mouse_directx_enable_hardware_cursor(int mode)
{
   /* Do nothing */
   (void)mode;
}



/* mouse_directx_select_system_cursor:
 *  Select an OS native cursor 
 */
static int mouse_directx_select_system_cursor (int cursor)
{
   HCURSOR wc;
   HWND allegro_wnd = win_get_window();
   
   wc = NULL;
   switch(cursor) {
      case MOUSE_CURSOR_ARROW:
         wc = LoadCursor(NULL, IDC_ARROW);
         break;
      case MOUSE_CURSOR_BUSY:
         wc = LoadCursor(NULL, IDC_WAIT);
         break;
      case MOUSE_CURSOR_QUESTION:
         wc = LoadCursor(NULL, IDC_HELP);
         break;
      case MOUSE_CURSOR_EDIT:
         wc = LoadCursor(NULL, IDC_IBEAM);
         break;
      default:
         return 0;
   }

   _win_hcursor = wc;
   SetCursor(_win_hcursor);
   PostMessage(allegro_wnd, WM_MOUSEMOVE, 0, 0);
   
   return cursor;
}
