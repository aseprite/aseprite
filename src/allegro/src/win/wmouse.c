/*
  Mouse driver for ASE
  by David Capello

  Based on code of Stefan Schimanski and Milan Mimica.
*/


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"

#include "wddraw.h"

#ifndef ALLEGRO_WINDOWS
   #error something is wrong with the makefile
#endif

#define PREFIX_I                "al-wmouse INFO: "
#define PREFIX_W                "al-wmouse WARNING: "
#define PREFIX_E                "al-wmouse ERROR: "


static MOUSE_DRIVER mouse_winapi;


#define MOUSE_WINAPI AL_ID('W','A','P','I')


_DRIVER_INFO _mouse_driver_list[] =
{
   {MOUSE_WINAPI, &mouse_winapi, TRUE},
   {MOUSEDRV_NONE, &mousedrv_none, TRUE},
   {0, NULL, 0}
};


static int mouse_winapi_init(void);
static void mouse_winapi_exit(void);
static void mouse_winapi_position(int x, int y);
static void mouse_winapi_set_range(int x1, int y1, int x2, int y2);
static void mouse_winapi_set_speed(int xspeed, int yspeed);
static void mouse_winapi_get_mickeys(int *mickeyx, int *mickeyy);
static void mouse_winapi_enable_hardware_cursor(int mode);
static int mouse_winapi_select_system_cursor(int cursor);


#define MOUSE_WINAPI AL_ID('W','A','P','I')


static MOUSE_DRIVER mouse_winapi =
{
   MOUSE_WINAPI,
   empty_string,
   empty_string,
   "WinAPI mouse",
   mouse_winapi_init,
   mouse_winapi_exit,
   NULL,                       // AL_METHOD(void, poll, (void));
   NULL,                       // AL_METHOD(void, timer_poll, (void));
   mouse_winapi_position,
   mouse_winapi_set_range,
   mouse_winapi_set_speed,
   mouse_winapi_get_mickeys,
   NULL,                       // AL_METHOD(int, analyse_data, (AL_CONST char *buffer, int size));
   mouse_winapi_enable_hardware_cursor,
   mouse_winapi_select_system_cursor
};


HCURSOR _win_hcursor = NULL;    /* Hardware cursor to display */



static int mouse_minx = 0;            /* mouse range */
static int mouse_miny = 0;
static int mouse_maxx = 319;
static int mouse_maxy = 199;

static int mymickey_x = 0;            /* for get_mouse_mickeys() */
static int mymickey_y = 0;
static int mymickey_ox = 0;
static int mymickey_oy = 0;

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
      mymickey_ox = 0;                        \
      mymickey_oy = 0;                        \
   }                                          \
}



/* mouse_set_syscursor: [window thread]
 *  Selects whatever system cursor we should display.
 */
int mouse_set_syscursor(void)
{
   if (_mouse_on || (gfx_driver && !gfx_driver->windowed))
      SetCursor(_win_hcursor);
   else
      SetCursor(LoadCursor(NULL, IDC_ARROW));

   return 0;
}



static int mouse_winapi_init(void)
{
   _mouse_on = TRUE;

   /* Returns the number of buttons available. */
   return GetSystemMetrics(SM_CMOUSEBUTTONS);
}



static void mouse_winapi_exit(void)
{
   /* Do nothing. */
}



/* mouse_winapi_position: [primary thread]
 */
static void mouse_winapi_position(int x, int y)
{
   _enter_critical();

   _mouse_x = x;
   _mouse_y = y;

   if (gfx_driver && gfx_driver->windowed)
      SetCursorPos(x+wnd_x, y+wnd_y);

   CLEAR_MICKEYS();

   mymickey_x = mymickey_y = 0;

   _exit_critical();
}



/* mouse_winapi_set_range: [primary thread]
 */
static void mouse_winapi_set_range(int x1, int y1, int x2, int y2)
{
   mouse_minx = x1;
   mouse_miny = y1;
   mouse_maxx = x2;
   mouse_maxy = y2;

   _enter_critical();

   _mouse_x = CLAMP(mouse_minx, _mouse_x, mouse_maxx);
   _mouse_y = CLAMP(mouse_miny, _mouse_y, mouse_maxy);

   CLEAR_MICKEYS();

   _exit_critical();
}



/* mouse_winapi_set_speed: [primary thread]
 */
static void mouse_winapi_set_speed(int xspeed, int yspeed)
{
   _enter_critical();

   CLEAR_MICKEYS();

   _exit_critical();
}



/* mouse_winapi_get_mickeys: [primary thread]
 */
static void mouse_winapi_get_mickeys(int *mickeyx, int *mickeyy)
{
   int temp_x = mymickey_x;
   int temp_y = mymickey_y;

   mymickey_x -= temp_x;
   mymickey_y -= temp_y;

   *mickeyx = temp_x;
   *mickeyy = temp_y;
}



/* mouse_winapi_enable_hardware_cursor:
 *  enable the hardware cursor; actually a no-op in Windows, but we need to
 *  put something in the vtable.
 */
static void mouse_winapi_enable_hardware_cursor(int mode)
{
   (void)mode;
}



/* mouse_winapi_select_system_cursor:
 *  Select an OS native cursor
 */
static int mouse_winapi_select_system_cursor(int cursor)
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
#ifdef ALLEGRO4_WITH_EXTRA_CURSORS
     case MOUSE_CURSOR_CROSS:
       wc = LoadCursor(NULL, IDC_CROSS);
       break;
     case MOUSE_CURSOR_MOVE:
       wc = LoadCursor(NULL, IDC_SIZEALL);
       break;
     case MOUSE_CURSOR_LINK:
       wc = LoadCursor(NULL, IDC_HAND);
       break;
     case MOUSE_CURSOR_FORBIDDEN:
       wc = LoadCursor(NULL, IDC_NO);
       break;
     case MOUSE_CURSOR_SIZE_N:
     case MOUSE_CURSOR_SIZE_S:
     case MOUSE_CURSOR_SIZE_NS:
       wc = LoadCursor(NULL, IDC_SIZENS);
       break;
     case MOUSE_CURSOR_SIZE_E:
     case MOUSE_CURSOR_SIZE_W:
     case MOUSE_CURSOR_SIZE_WE:
       wc = LoadCursor(NULL, IDC_SIZEWE);
       break;
     case MOUSE_CURSOR_SIZE_NW:
     case MOUSE_CURSOR_SIZE_SE:
       wc = LoadCursor(NULL, IDC_SIZENWSE);
       break;
     case MOUSE_CURSOR_SIZE_NE:
     case MOUSE_CURSOR_SIZE_SW:
       wc = LoadCursor(NULL, IDC_SIZENESW);
       break;
#endif
     default:
       return 0;
   }

   _win_hcursor = wc;
   SetCursor(_win_hcursor);

   return cursor;
}



void _al_win_mouse_handle_button(HWND hwnd, int button, BOOL down, int x, int y, BOOL abs)
{
  int last_mouse_b;

   _enter_critical();

   if (down)
      _mouse_b |= (1 << (button-1));
   else
      _mouse_b &= ~(1 << (button-1));

   last_mouse_b = _mouse_b;

   _exit_critical();

#if 0 /* Aseprite: We handle mouse capture in the she and ui layers */

   /* If there is a mouse button pressed we capture the mouse, in any
      other cases we release it. */
   if (last_mouse_b) {
     if (GetCapture() != hwnd)
       SetCapture(hwnd);
   }
   else {
     if (GetCapture() == hwnd)
       ReleaseCapture();
   }

#endif

   _handle_mouse_input();
}



void _al_win_mouse_handle_wheel(HWND hwnd, int z, BOOL abs)
{
   _enter_critical();

   _mouse_z += z;

   _exit_critical();

   _handle_mouse_input();
}



void _al_win_mouse_handle_move(HWND hwnd, int x, int y)
{
   _enter_critical();

   /* Try to restore the primary surface as soon as possible if we
      have lost it and were not able to recreate it */
   if (gfx_directx_primary_surface && gfx_directx_primary_surface->id == 0) {
     restore_all_ddraw_surfaces();
   }

   _mouse_x = CLAMP(mouse_minx, x, mouse_maxx);
   _mouse_y = CLAMP(mouse_miny, y, mouse_maxy);

   mymickey_x += x - mymickey_ox;
   mymickey_y += y - mymickey_oy;

   mymickey_ox = x;
   mymickey_oy = y;

   _exit_critical();

   _handle_mouse_input();
}
