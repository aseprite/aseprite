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
 *      Windows keyboard driver.
 *
 *      By Stefan Schimanski, hacked up by Peter Wang and Elias Pschernig.
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

#define PREFIX_I                "al-wkey INFO: "
#define PREFIX_W                "al-wkey WARNING: "
#define PREFIX_E                "al-wkey ERROR: "


#define DINPUT_BUFFERSIZE 256
static HANDLE key_input_event = NULL;
static HANDLE key_input_processed_event = NULL;
static LPDIRECTINPUT key_dinput = NULL;
static LPDIRECTINPUTDEVICE key_dinput_device = NULL;


/* lookup table for converting DIK_* scancodes into Allegro KEY_* codes */
/* this table was from pckeys.c  */
static const unsigned char hw_to_mycode[256] = {
   /* 0x00 */ 0, KEY_ESC, KEY_1, KEY_2,
   /* 0x04 */ KEY_3, KEY_4, KEY_5, KEY_6,
   /* 0x08 */ KEY_7, KEY_8, KEY_9, KEY_0,
   /* 0x0C */ KEY_MINUS, KEY_EQUALS, KEY_BACKSPACE, KEY_TAB,
   /* 0x10 */ KEY_Q, KEY_W, KEY_E, KEY_R,
   /* 0x14 */ KEY_T, KEY_Y, KEY_U, KEY_I,
   /* 0x18 */ KEY_O, KEY_P, KEY_OPENBRACE, KEY_CLOSEBRACE,
   /* 0x1C */ KEY_ENTER, KEY_LCONTROL, KEY_A, KEY_S,
   /* 0x20 */ KEY_D, KEY_F, KEY_G, KEY_H,
   /* 0x24 */ KEY_J, KEY_K, KEY_L, KEY_SEMICOLON,
   /* 0x28 */ KEY_QUOTE, KEY_TILDE, KEY_LSHIFT, KEY_BACKSLASH,
   /* 0x2C */ KEY_Z, KEY_X, KEY_C, KEY_V,
   /* 0x30 */ KEY_B, KEY_N, KEY_M, KEY_COMMA,
   /* 0x34 */ KEY_STOP, KEY_SLASH, KEY_RSHIFT, KEY_ASTERISK,
   /* 0x38 */ KEY_ALT, KEY_SPACE, KEY_CAPSLOCK, KEY_F1,
   /* 0x3C */ KEY_F2, KEY_F3, KEY_F4, KEY_F5,
   /* 0x40 */ KEY_F6, KEY_F7, KEY_F8, KEY_F9,
   /* 0x44 */ KEY_F10, KEY_NUMLOCK, KEY_SCRLOCK, KEY_7_PAD,
   /* 0x48 */ KEY_8_PAD, KEY_9_PAD, KEY_MINUS_PAD, KEY_4_PAD,
   /* 0x4C */ KEY_5_PAD, KEY_6_PAD, KEY_PLUS_PAD, KEY_1_PAD,
   /* 0x50 */ KEY_2_PAD, KEY_3_PAD, KEY_0_PAD, KEY_DEL_PAD,
   /* 0x54 */ KEY_PRTSCR, 0, KEY_BACKSLASH2, KEY_F11,
   /* 0x58 */ KEY_F12, 0, 0, KEY_LWIN,
   /* 0x5C */ KEY_RWIN, KEY_MENU, 0, 0,
   /* 0x60 */ 0, 0, 0, 0,
   /* 0x64 */ 0, 0, 0, 0,
   /* 0x68 */ 0, 0, 0, 0,
   /* 0x6C */ 0, 0, 0, 0,
   /* 0x70 */ KEY_KANA, 0, 0, KEY_ABNT_C1,
   /* 0x74 */ 0, 0, 0, 0,
   /* 0x78 */ 0, KEY_CONVERT, 0, KEY_NOCONVERT,
   /* 0x7C */ 0, KEY_YEN, 0, 0,
   /* 0x80 */ 0, 0, 0, 0,
   /* 0x84 */ 0, 0, 0, 0,
   /* 0x88 */ 0, 0, 0, 0,
   /* 0x8C */ 0, 0, 0, 0,
   /* 0x90 */ 0, KEY_AT, KEY_COLON2, 0,
   /* 0x94 */ KEY_KANJI, 0, 0, 0,
   /* 0x98 */ 0, 0, 0, 0,
   /* 0x9C */ KEY_ENTER_PAD, KEY_RCONTROL, 0, 0,
   /* 0xA0 */ 0, 0, 0, 0,
   /* 0xA4 */ 0, 0, 0, 0,
   /* 0xA8 */ 0, 0, 0, 0,
   /* 0xAC */ 0, 0, 0, 0,
   /* 0xB0 */ 0, 0, 0, 0,
   /* 0xB4 */ 0, KEY_SLASH_PAD, 0, KEY_PRTSCR,
   /* 0xB8 */ KEY_ALTGR, 0, 0, 0,
   /* 0xBC */ 0, 0, 0, 0,
   /* 0xC0 */ 0, 0, 0, 0,
   /* 0xC4 */ 0, KEY_PAUSE, 0, KEY_HOME,
   /* 0xC8 */ KEY_UP, KEY_PGUP, 0, KEY_LEFT,
   /* 0xCC */ 0, KEY_RIGHT, 0, KEY_END,
   /* 0xD0 */ KEY_DOWN, KEY_PGDN, KEY_INSERT, KEY_DEL,
   /* 0xD4 */ 0, 0, 0, 0,
   /* 0xD8 */ 0, 0, 0, KEY_LWIN,
   /* 0xDC */ KEY_RWIN, KEY_MENU, 0, 0,
   /* 0xE0 */ 0, 0, 0, 0,
   /* 0xE4 */ 0, 0, 0, 0,
   /* 0xE8 */ 0, 0, 0, 0,
   /* 0xEC */ 0, 0, 0, 0,
   /* 0xF0 */ 0, 0, 0, 0,
   /* 0xF4 */ 0, 0, 0, 0,
   /* 0xF8 */ 0, 0, 0, 0,
   /* 0xFC */ 0, 0, 0, 0
};

/* Used in scancode_to_name. */
static LONG reverse_mapping[KEY_MAX];


/* dinput_err_str:
 *  Returns a DirectInput error string.
 */
#ifdef DEBUGMODE
static char* dinput_err_str(long err)
{
   static char err_str[64];

   switch (err) {

      case DIERR_NOTACQUIRED:
         _al_sane_strncpy(err_str, "the device is not acquired", sizeof(err_str));
         break;

      case DIERR_INPUTLOST:
         _al_sane_strncpy(err_str, "access to the device was lost", sizeof(err_str));
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



/* Update the key_shifts.
 */
static void update_shifts(BYTE * keystate)
{
   /* TODO: There must be a more efficient way to maintain key_modifiers? */
   /* Can't we just deprecate key_shifts, now that pckeys.c is gone? EP */
   unsigned int modifiers = 0;

   if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
       modifiers |= KB_SHIFT_FLAG;
   if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
       modifiers |= KB_CTRL_FLAG;
   if (GetAsyncKeyState(VK_MENU) & 0x8000)
       modifiers |= KB_ALT_FLAG;

   if (keystate[VK_SCROLL] & 1)
      modifiers |= KB_SCROLOCK_FLAG;
   if (keystate[VK_NUMLOCK] & 1)
      modifiers |= KB_NUMLOCK_FLAG;
   if (keystate[VK_CAPITAL] & 1)
      modifiers |= KB_CAPSLOCK_FLAG;

   _key_shifts = modifiers;

}



/* handle_key_press: [input thread]
 *  Does stuff when a key is pressed.
 */
static void handle_key_press(unsigned char scancode)
{
   int mycode;
   int unicode;
   int n;
   UINT vkey;
   BYTE keystate[256];
   WCHAR chars[16];

   vkey = MapVirtualKey(scancode, 1);

   GetKeyboardState(keystate);
   update_shifts(keystate);

   /* TODO: shouldn't we base the mapping on vkey? */
   mycode = hw_to_mycode[scancode];
   if (mycode == 0)
      return;

   /* MapVirtualKey always returns the arrow key VKEY, so adjust
      it if num lock is on */
   if (keystate[VK_NUMLOCK]) {
      switch (scancode) {
         case DIK_NUMPAD0:
            vkey = VK_NUMPAD0;
            break;
         case DIK_NUMPAD1:
            vkey = VK_NUMPAD1;
            break;
         case DIK_NUMPAD2:
            vkey = VK_NUMPAD2;
            break;
         case DIK_NUMPAD3:
            vkey = VK_NUMPAD3;
            break;
         case DIK_NUMPAD4:
            vkey = VK_NUMPAD4;
            break;
         case DIK_NUMPAD5:
            vkey = VK_NUMPAD5;
            break;
         case DIK_NUMPAD6:
            vkey = VK_NUMPAD6;
            break;
         case DIK_NUMPAD7:
            vkey = VK_NUMPAD7;
            break;
         case DIK_NUMPAD8:
            vkey = VK_NUMPAD8;
            break;
         case DIK_NUMPAD9:
            vkey = VK_NUMPAD9;
            break;
         case DIK_DECIMAL:
            vkey = VK_DECIMAL;
            break;
     }
   }

   /* what's life without a few special cases */
   switch (scancode) {
      case DIK_DIVIDE:
         vkey = VK_DIVIDE;
         break;
      case DIK_MULTIPLY:
         vkey = VK_MULTIPLY;
         break;
      case DIK_SUBTRACT:
         vkey = VK_SUBTRACT;
         break;
      case DIK_ADD:
         vkey = VK_ADD;
         break;
      case DIK_NUMPADENTER:
         vkey = VK_RETURN;
   }

   /* TODO: is there an advantage using ToUnicode? maybe it would allow
    * Chinese and so on characters? For now, always ToAscii is used. */
   //n = ToUnicode(vkey, scancode, keystate, chars, 16, 0);
   n = ToAscii(vkey, scancode, keystate, (WORD *)chars, 0);
   if (n == 1)
   {
      WCHAR wstr[2];
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCSTR)chars, n, wstr, 2);
      unicode = wstr[0];
   }
   else
   {
      /* Don't generate key presses for modifier keys or dead keys */
      if (mycode >= KEY_MODIFIERS || n != 0)
	 unicode = -1;
      else
	 unicode = 0;
   }

   /* When alt key is pressed, any key always must return ASCII 0 in Allegro. */
   if (unicode > 0 && (keystate[VK_LMENU] & 0x80)) {
      unicode = 0;
   }

   _handle_key_press(unicode, mycode);
}



/* handle_key_release: [input thread]
 *  Does stuff when a key is released.  The keyboard event source
 *  should be locked.
 */
static void handle_key_release(unsigned char scancode)
{
   int mycode = hw_to_mycode[scancode];
   if (mycode == 0)
      return;

   {
      BYTE keystate[256];
      GetKeyboardState(keystate);
      update_shifts(keystate);
   }

   _handle_key_release(mycode);
}



/* key_dinput_handle_scancode: [input thread]
 *  Handles a single scancode.
 */
static void key_dinput_handle_scancode(unsigned char scancode, int pressed)
{
   HWND allegro_wnd = win_get_window();
   static int ignore_three_finger_flag = FALSE;
   /* ignore special Windows keys (alt+tab, alt+space, (ctrl|alt)+esc) */
   if (((scancode == DIK_TAB) && (_key_shifts & KB_ALT_FLAG))
       || ((scancode == DIK_SPACE) && (_key_shifts & KB_ALT_FLAG))
       || ((scancode == DIK_ESCAPE) && (_key_shifts & (KB_CTRL_FLAG | KB_ALT_FLAG))))
      return;

   /* alt+F4 triggers a WM_CLOSE under Windows */
   if ((scancode == DIK_F4) && (_key_shifts & KB_ALT_FLAG)) {
      if (pressed)
         PostMessage(allegro_wnd, WM_CLOSE, 0, 0);
      return;
   }

   /* Special case KEY_PAUSE as flip-flop key. */
   if (scancode == DIK_PAUSE) {
      if (!pressed)
         return;
      if (key[KEY_PAUSE])
         pressed = 0;
      else
         pressed = 1;
   }

   /* if not foreground, filter out press codes and handle only release codes */
   if (!wnd_sysmenu || !pressed) {
      /* three-finger salute for killing the program */
      if (three_finger_flag && (_key_shifts & KB_CTRL_FLAG) && (_key_shifts & KB_ALT_FLAG)) {
         if (scancode == 0x00) {
            /* when pressing CTRL-ALT-DEL, Windows launches CTRL-ALT-EVERYTHING */
            ignore_three_finger_flag = TRUE;
	 }
	 else if (!ignore_three_finger_flag && (scancode == DIK_END || scancode == DIK_NUMPAD1)) {
	    /* we can now safely assume the user hit CTRL-ALT-END as opposed to CTRL-ALT-DEL */
	    _TRACE(PREFIX_I "Terminating application\n");
	    abort();
	 }
	 else if (ignore_three_finger_flag && scancode == 0xff) {
	    /* Windows is finished with CTRL-ALT-EVERYTHING - lets return to normality */
	    ignore_three_finger_flag = FALSE;
	    _key_shifts = 0;
	 }
      }

      if (pressed)
         handle_key_press(scancode);
      else
         handle_key_release(scancode);
   }
}



/* key_dinput_handle: [input thread]
 *  Handles queued keyboard input.
 */
static void key_dinput_handle(void)
{
   static DIDEVICEOBJECTDATA scancode_buffer[DINPUT_BUFFERSIZE];
   unsigned long int waiting_scancodes;
   HRESULT hr;
   unsigned long int i;

   /* the whole buffer is free */
   waiting_scancodes = DINPUT_BUFFERSIZE;

   /* fill the buffer */
   hr = IDirectInputDevice_GetDeviceData(key_dinput_device,
                                         sizeof(DIDEVICEOBJECTDATA),
                                         scancode_buffer,
                                         &waiting_scancodes,
                                         0);

   /* was device lost ? */
   if ((hr == DIERR_NOTACQUIRED) || (hr == DIERR_INPUTLOST)) {
      /* reacquire device */
      _TRACE(PREFIX_W "keyboard device not acquired or lost\n");
      wnd_schedule_proc(key_dinput_acquire);
   }
   else if (FAILED(hr)) {  /* other error? */
      _TRACE(PREFIX_E "unexpected error while filling keyboard scancode buffer\n");
   }
   else {
      /* normally only this case should happen */
      for (i = 0; i < waiting_scancodes; i++) {
         key_dinput_handle_scancode((unsigned char) scancode_buffer[i].dwOfs,
                                    scancode_buffer[i].dwData & 0x80);
      }
   }

   SetEvent(key_input_processed_event);
}



/* key_dinput_acquire: [window thread]
 *  Acquires the keyboard device. This must be called after a
 *  window switch for example if the device is in foreground
 *  cooperative level.
 */
int key_dinput_acquire(void)
{
   HRESULT hr;
   BYTE keystate[256];

   if (key_dinput_device) {
      /* Read the current Windows keyboard state */
      GetKeyboardState(keystate);
      update_shifts(keystate);

      hr = IDirectInputDevice_Acquire(key_dinput_device);

      if (FAILED(hr)) {
         _TRACE(PREFIX_E "acquire keyboard failed: %s\n", dinput_err_str(hr));
         return -1;
      }

      /* Initialize keyboard state */
      SetEvent(key_input_event);

      return 0;
   }
   else
      return -1;
}



/* key_dinput_unacquire: [window thread]
 *  Unacquires the keyboard device.
 */
int key_dinput_unacquire(void)
{
   int key;

   if (key_dinput_device) {
      IDirectInputDevice_Unacquire(key_dinput_device);

      /* release all keys */
      for (key=0; key<256; key++)
         if (key != KEY_PAUSE)
            key_dinput_handle_scancode((unsigned char) key, FALSE);

      return 0;
   }
   else
      return -1;
}



/* key_dinput_exit: [primary thread]
 *  Shuts down the DirectInput keyboard device.
 */
static int key_dinput_exit(void)
{
   if (key_dinput_device) {
      /* unregister event handler first */
      _win_input_unregister_event(key_input_event);

      /* unacquire device */
      wnd_call_proc(key_dinput_unacquire);

      /* now it can be released */
      IDirectInputDevice_Release(key_dinput_device);
      key_dinput_device = NULL;
   }

   /* release DirectInput interface */
   if (key_dinput) {
      IDirectInput_Release(key_dinput);
      key_dinput = NULL;
   }

   /* close event handle */
   if (key_input_event) {
      CloseHandle(key_input_event);
      key_input_event = NULL;
   }

   return 0;
}



/* get_reverse_mapping:
 *  Build a table that maps Allegro "scancodes" to arguments to
 *  GetKeyNameText() that will return the name of the key.
 *  The format of the argument is given in the MSDN library, e.g. the
 *  `lparam' argument for the WM_KEYDOWN message.  Most importantly:
 *    bits 16-23:
 *       Specifies the scan code.
 *    bit 24:
 *       Specifies whether the key is an extended key, such as the right-hand
 *       ALT and CTRL keys that appear on an enhanced 101- or 102-key keyboard.
 *       The value is 1 if it is an extended key; otherwise, it is 0.
 *
 *  We used to attempt to derive this table from `hw_to_mycode' but it is not
 *  straight forward.  For example to get the name of the right Ctrl key we
 *  need to pass the same scancode as for the left Ctrl key, but with the
 *  extended key flag set.  The values below come from printing out the
 *  value of the `lparam' argument for each WM_KEYDOWN message received.
 *  There is code in wwnd.c to help someone else do this for more keys.
 */
static void get_reverse_mapping(void)
{
#define REV reverse_mapping

   REV[KEY_A] = 0x01E0000;
   REV[KEY_B] = 0x0300000;
   REV[KEY_C] = 0x02E0000;
   REV[KEY_D] = 0x0200000;
   REV[KEY_E] = 0x0120000;
   REV[KEY_F] = 0x0210000;
   REV[KEY_G] = 0x0220000;
   REV[KEY_H] = 0x0230000;
   REV[KEY_I] = 0x0170000;
   REV[KEY_J] = 0x0240000;
   REV[KEY_K] = 0x0250000;
   REV[KEY_L] = 0x0260000;
   REV[KEY_M] = 0x0320000;
   REV[KEY_N] = 0x0310000;
   REV[KEY_O] = 0x0180000;
   REV[KEY_P] = 0x0190000;
   REV[KEY_Q] = 0x0100000;
   REV[KEY_R] = 0x0130000;
   REV[KEY_S] = 0x01F0000;
   REV[KEY_T] = 0x0140000;
   REV[KEY_U] = 0x0160000;
   REV[KEY_V] = 0x02F0000;
   REV[KEY_W] = 0x0110000;
   REV[KEY_X] = 0x02D0000;
   REV[KEY_Y] = 0x0150000;
   REV[KEY_Z] = 0x02C0000;
   REV[KEY_0] = 0x00B0000;
   REV[KEY_1] = 0x0020000;
   REV[KEY_2] = 0x0030000;
   REV[KEY_3] = 0x0040000;
   REV[KEY_4] = 0x0050000;
   REV[KEY_5] = 0x0060000;
   REV[KEY_6] = 0x0070000;
   REV[KEY_7] = 0x0080000;
   REV[KEY_8] = 0x0090000;
   REV[KEY_9] = 0x00A0000;
   REV[KEY_0_PAD] = 0x0520000;
   REV[KEY_1_PAD] = 0x04F0000;
   REV[KEY_2_PAD] = 0x0500000;
   REV[KEY_3_PAD] = 0x0510000;
   REV[KEY_4_PAD] = 0x04B0000;
   REV[KEY_5_PAD] = 0x04C0000;
   REV[KEY_6_PAD] = 0x04D0000;
   REV[KEY_7_PAD] = 0x0470000;
   REV[KEY_8_PAD] = 0x0480000;
   REV[KEY_9_PAD] = 0x0490000;
   REV[KEY_F1] = 0x03B0000;
   REV[KEY_F2] = 0x03C0000;
   REV[KEY_F3] = 0x03D0000;
   REV[KEY_F4] = 0x03E0000;
   REV[KEY_F5] = 0x03F0000;
   REV[KEY_F6] = 0x0400000;
   REV[KEY_F7] = 0x0410000;
   REV[KEY_F8] = 0x0420000;
   REV[KEY_F9] = 0x0430000;
   REV[KEY_F10] = 0x0440000;
   REV[KEY_F11] = 0x0570000;
   REV[KEY_F12] = 0x0580000;
   REV[KEY_ESC] = 0x0010000;
   REV[KEY_TILDE] = 0x0290000;
   REV[KEY_MINUS] = 0x00C0000;
   REV[KEY_EQUALS] = 0x00D0000;
   REV[KEY_BACKSPACE] = 0x00E0000;
   REV[KEY_TAB] = 0x00F0000;
   REV[KEY_OPENBRACE] = 0x01A0000;
   REV[KEY_CLOSEBRACE] = 0x01B0000;
   REV[KEY_ENTER] = 0x01C0000;
   REV[KEY_COLON] = 0x0270000;
   REV[KEY_QUOTE] = 0x0280000;
   REV[KEY_BACKSLASH] = 0x02B0000;
   /* KEY_BACKSLASH2 */
   REV[KEY_COMMA] = 0x0330000;
   REV[KEY_STOP] = 0x0340000;
   REV[KEY_SLASH] = 0x0350000;
   REV[KEY_SPACE] = 0x0390000;
   REV[KEY_INSERT] = 0x1520000;
   REV[KEY_DEL] = 0x1530000;
   REV[KEY_HOME] = 0x1470000;
   REV[KEY_END] = 0x14F0000;
   REV[KEY_PGUP] = 0x1490000;
   REV[KEY_PGDN] = 0x1510000;
   REV[KEY_LEFT] = 0x14B0000;
   REV[KEY_RIGHT] = 0x14D0000;
   REV[KEY_UP] = 0x1480000;
   REV[KEY_DOWN] = 0x1500000;
   REV[KEY_SLASH_PAD] = 0x1350000;
   REV[KEY_ASTERISK] = 0x0370000;
   REV[KEY_MINUS_PAD] = 0x04A0000;
   REV[KEY_PLUS_PAD] = 0x04E0000;
   REV[KEY_DEL_PAD] = 0x0530000;
   REV[KEY_ENTER_PAD] = 0x11C0000;
   /* XXX doesn't work
   REV[KEY_PRTSCR] = 0x570000;
   */
   REV[KEY_PAUSE] = 0x0450000;
   /* XXX Japanese keys missing
   REV[KEY_ABNT_C1] =
   REV[KEY_YEN] =
   REV[KEY_KANA] =
   REV[KEY_CONVERT] =
   REV[KEY_NOCONVERT] =
   REV[KEY_AT] =
   REV[KEY_CIRCUMFLEX] =
   REV[KEY_COLON2] =
   REV[KEY_KANJI] =
   */

   REV[KEY_LSHIFT] = 0x02A0000;
   REV[KEY_RSHIFT] = 0x0360000;
   REV[KEY_LCONTROL] = 0x01D0000;
   REV[KEY_RCONTROL] = 0x11D0000;
   REV[KEY_ALT]	= 0x0380000;
   REV[KEY_ALTGR] = 0x1380000;
   REV[KEY_LWIN] = 0x15B0000;
   REV[KEY_RWIN] = 0x15C0000;
   REV[KEY_MENU] = 0x15D0000;
   REV[KEY_SCRLOCK] = 0x0460000;
   REV[KEY_NUMLOCK] = 0x1450000;
   REV[KEY_CAPSLOCK] = 0x03A0000;

#undef REV
}



/* key_dinput_init: [primary thread]
 *  Sets up the DirectInput keyboard device.
 */
static int key_dinput_init(void)
{
   HRESULT hr;
   HWND allegro_wnd = win_get_window();
   DIPROPDWORD property_buf_size =
   {
      /* the header */
      {
         sizeof(DIPROPDWORD),  // diph.dwSize
         sizeof(DIPROPHEADER), // diph.dwHeaderSize
         0,                     // diph.dwObj
         DIPH_DEVICE,           // diph.dwHow
      },

      /* the data */
      DINPUT_BUFFERSIZE,         // dwData
   };

   /* Get DirectInput interface */
   hr = CoCreateInstance(&CLSID_DirectInput, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectInput, &key_dinput);
   if (FAILED(hr))
      goto Error;

   hr = IDirectInput_Initialize(key_dinput, allegro_inst, DIRECTINPUT_VERSION);
   if (FAILED(hr))
      goto Error;

   /* Create the keyboard device */
   hr = IDirectInput_CreateDevice(key_dinput, &GUID_SysKeyboard, &key_dinput_device, NULL);
   if (FAILED(hr))
      goto Error;

   /* Set data format */
   hr = IDirectInputDevice_SetDataFormat(key_dinput_device, &c_dfDIKeyboard);
   if (FAILED(hr))
      goto Error;

   /* Set buffer size */
   hr = IDirectInputDevice_SetProperty(key_dinput_device, DIPROP_BUFFERSIZE, &property_buf_size.diph);
   if (FAILED(hr))
      goto Error;

   /* Set cooperative level */
   hr = IDirectInputDevice_SetCooperativeLevel(key_dinput_device, allegro_wnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
   if (FAILED(hr))
      goto Error;

   /* Enable event notification */
   key_input_event = CreateEvent(NULL, FALSE, FALSE, NULL);
   hr = IDirectInputDevice_SetEventNotification(key_dinput_device, key_input_event);
   if (FAILED(hr))
      goto Error;

   /* Register event handler */
   if (_win_input_register_event(key_input_event, key_dinput_handle) != 0)
      goto Error;

   get_reverse_mapping();

   /* Acquire the device */
   wnd_call_proc(key_dinput_acquire);

   return 0;

 Error:
   key_dinput_exit();
   return -1;
}



/* key_directx_init: [primary thread]
 */
static int key_directx_init(void)
{
   /* keyboard input is handled by the window thread */
   key_input_processed_event = CreateEvent(NULL, FALSE, FALSE, NULL);

   if (key_dinput_init() != 0) {
      /* something has gone wrong */
      _TRACE(PREFIX_E "keyboard handler init failed\n");
      CloseHandle(key_input_processed_event);
      return -1;
   }

   /* the keyboard handler has successfully set up */
   _TRACE(PREFIX_I "keyboard handler starts\n");
   return 0;
}



/* key_directx_exit: [primary thread]
 */
static void key_directx_exit(void)
{
   if (key_dinput_device) {
      /* command keyboard handler shutdown */
      _TRACE(PREFIX_I "keyboard handler exits\n");
      key_dinput_exit();

      /* now we can free all resources */
      CloseHandle(key_input_processed_event);
   }
}



/* key_directx_wait_for_input: [primary thread]
 */
static void key_directx_wait_for_input(void)
{
   WaitForSingleObject(key_input_processed_event, INFINITE);
}



/* static void key_directx_stop_wait: [primary thread]
 */
static void key_directx_stop_wait(void)
{
   SetEvent(key_input_processed_event);
}



/* scancode_to_name:
 *  Converts the given scancode to a description of the key.
 */
static AL_CONST char *key_directx_scancode_to_name(const int scancode)
{
   static char name[256];
   TCHAR str[256];
   WCHAR wstr[256];

   ASSERT(scancode >= 0 && scancode < KEY_MAX);

   if (GetKeyNameText(reverse_mapping[scancode], str, sizeof str)) {
      /* let Windows translate from the current encoding into UTF16 */
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str, -1, wstr, sizeof wstr);
      /* translate from utf16 to Allegro's current encoding */
      uconvert((char *)wstr, U_UNICODE, name, U_CURRENT, sizeof name);
      /* why oh why doesn't everybody just use UTF8/16 */
      return name;
   }
   else
      return _keyboard_common_names[scancode];
}



static KEYBOARD_DRIVER keyboard_directx =
{
   KEYBOARD_DIRECTX,
   0,
   0,
   "DirectInput keyboard",
   1,
   key_directx_init,
   key_directx_exit,
   NULL, NULL, NULL,
   key_directx_wait_for_input,
   key_directx_stop_wait,
   NULL,
   key_directx_scancode_to_name
};



_DRIVER_INFO _keyboard_driver_list[] =
{
   {KEYBOARD_DIRECTX, &keyboard_directx, TRUE},
   {0, NULL, 0}
};
