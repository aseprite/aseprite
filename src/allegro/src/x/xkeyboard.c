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
 *      X keyboard driver.
 *
 *      By Elias Pschernig.
 *
 *      See readme.txt for copyright information.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <X11/Xlocale.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>

#include "allegro.h"
#include "xalleg.h"
#include "allegro/internal/aintern.h"
#include "xwin.h"

#define PREFIX_I                "al-xkey INFO: "
#define PREFIX_W                "al-xkey WARNING: "
#define PREFIX_E                "al-xkey ERROR: "

#ifdef ALLEGRO_XWINDOWS_WITH_XIM
static XIM xim = NULL;
static XIC xic = NULL;
#endif
static XModifierKeymap *xmodmap = NULL;
static int xkeyboard_installed = 0;
static int used[KEY_MAX];
static int sym_per_key;
static int min_keycode, max_keycode;
static KeySym *keysyms = NULL;
static int main_pid; /* The pid to kill with ctrl-alt-del. */
static int pause_key = 0; /* Allegro's special pause key state. */

/* This table can be ammended to provide more reasonable defaults for
 * mappings other than US/UK. They are used to map X11 KeySyms as found in
 * X11/keysym.h to Allegro's KEY_* codes. This will only work well on US/UK
 * keyboards since Allegro simply doesn't have KEY_* codes for non US/UK
 * keyboards. So with other mappings, the unmapped keys will be distributed
 * arbitrarily to the remaining KEY_* codes.
 *
 * Double mappings should be avoided, else they can lead to different keys
 * producing the same KEY_* code on some mappings.
 *
 * In cases where there is no other way to detect a key, since we have no
 * ASCII applied to it, like KEY_LEFT or KEY_PAUSE, multiple mappings should
 * be ok though. This table will never be able to be 100% perfect, so just
 * try to make it work for as many as possible, using additional hacks in
 * some cases. There is also still the possibility to override keys with
 * the [xkeyboard] config section, so users can always re-map keys. (E.g.
 * to play an Allegro game which hard-coded KEY_Y and KEY_X for left and
 * right.)
 */
static struct {
   KeySym keysym;
   int allegro_key;
}
translation_table[] = {
   {XK_a, KEY_A},
   {XK_b, KEY_B},
   {XK_c, KEY_C},
   {XK_d, KEY_D},
   {XK_e, KEY_E},
   {XK_f, KEY_F},
   {XK_g, KEY_G},
   {XK_h, KEY_H},
   {XK_i, KEY_I},
   {XK_j, KEY_J},
   {XK_k, KEY_K},
   {XK_l, KEY_L},
   {XK_m, KEY_M},
   {XK_n, KEY_N},
   {XK_o, KEY_O},
   {XK_p, KEY_P},
   {XK_q, KEY_Q},
   {XK_r, KEY_R},
   {XK_s, KEY_S},
   {XK_t, KEY_T},
   {XK_u, KEY_U},
   {XK_v, KEY_V},
   {XK_w, KEY_W},
   {XK_x, KEY_X},
   {XK_y, KEY_Y},
   {XK_z, KEY_Z},
   {XK_0, KEY_0},
   {XK_1, KEY_1},
   {XK_2, KEY_2},
   {XK_3, KEY_3},
   {XK_4, KEY_4},
   {XK_5, KEY_5},
   {XK_6, KEY_6},
   {XK_7, KEY_7},
   {XK_8, KEY_8},
   {XK_9, KEY_9},

   /* Double mappings for numeric keyboard.
    * If an X server actually uses both at the same time, Allegro will
    * detect them as the same. But normally, an X server just reports it as
    * either of them, and therefore we always get the keys as KEY_*_PAD.
    */
   {XK_KP_0, KEY_0_PAD},
   {XK_KP_Insert, KEY_0_PAD},
   {XK_KP_1, KEY_1_PAD},
   {XK_KP_End, KEY_1_PAD},
   {XK_KP_2, KEY_2_PAD},
   {XK_KP_Down, KEY_2_PAD},
   {XK_KP_3, KEY_3_PAD},
   {XK_KP_Page_Down, KEY_3_PAD},
   {XK_KP_4, KEY_4_PAD},
   {XK_KP_Left, KEY_4_PAD},
   {XK_KP_5, KEY_5_PAD},
   {XK_KP_Begin, KEY_5_PAD},
   {XK_KP_6, KEY_6_PAD},
   {XK_KP_Right, KEY_6_PAD},
   {XK_KP_7, KEY_7_PAD},
   {XK_KP_Home, KEY_7_PAD},
   {XK_KP_8, KEY_8_PAD},
   {XK_KP_Up, KEY_8_PAD},
   {XK_KP_9, KEY_9_PAD},
   {XK_KP_Page_Up, KEY_9_PAD},
   {XK_KP_Delete, KEY_DEL_PAD},
   {XK_KP_Decimal, KEY_DEL_PAD},

   /* Double mapping!
    * Same as above - but normally, the X server just reports one or the
    * other for the Pause key, and the other is not reported for any key.
    */
   {XK_Pause, KEY_PAUSE},
   {XK_Break, KEY_PAUSE},

   {XK_F1, KEY_F1},
   {XK_F2, KEY_F2},
   {XK_F3, KEY_F3},
   {XK_F4, KEY_F4},
   {XK_F5, KEY_F5},
   {XK_F6, KEY_F6},
   {XK_F7, KEY_F7},
   {XK_F8, KEY_F8},
   {XK_F9, KEY_F9},
   {XK_F10, KEY_F10},
   {XK_F11, KEY_F11},
   {XK_F12, KEY_F12},
   {XK_Escape, KEY_ESC},
   {XK_grave, KEY_TILDE}, /* US left of 1 */
   {XK_minus, KEY_MINUS}, /* US right of 0 */
   {XK_equal, KEY_EQUALS}, /* US 2 right of 0 */
   {XK_BackSpace, KEY_BACKSPACE},
   {XK_Tab, KEY_TAB},
   {XK_bracketleft, KEY_OPENBRACE}, /* US right of P */
   {XK_bracketright, KEY_CLOSEBRACE}, /* US 2 right of P */
   {XK_Return, KEY_ENTER},
   {XK_semicolon, KEY_COLON}, /* US right of L */
   {XK_apostrophe, KEY_QUOTE}, /* US 2 right of L */
   {XK_backslash, KEY_BACKSLASH}, /* US 3 right of L */
   {XK_less, KEY_BACKSLASH2}, /* US left of Y */
   {XK_comma, KEY_COMMA}, /* US right of M */
   {XK_period, KEY_STOP}, /* US 2 right of M */
   {XK_slash, KEY_SLASH}, /* US 3 right of M */
   {XK_space, KEY_SPACE},
   {XK_Insert, KEY_INSERT},
   {XK_Delete, KEY_DEL},
   {XK_Home, KEY_HOME},
   {XK_End, KEY_END},
   {XK_Page_Up, KEY_PGUP},
   {XK_Page_Down, KEY_PGDN},
   {XK_Left, KEY_LEFT},
   {XK_Right, KEY_RIGHT},
   {XK_Up, KEY_UP},
   {XK_Down, KEY_DOWN},
   {XK_KP_Divide, KEY_SLASH_PAD},
   {XK_KP_Multiply, KEY_ASTERISK},
   {XK_KP_Subtract, KEY_MINUS_PAD},
   {XK_KP_Add, KEY_PLUS_PAD},
   {XK_KP_Enter, KEY_ENTER_PAD},
   {XK_Print, KEY_PRTSCR},

   //{, KEY_ABNT_C1},
   //{, KEY_YEN},
   //{, KEY_KANA},
   //{, KEY_CONVERT},
   //{, KEY_NOCONVERT},
   //{, KEY_AT},
   //{, KEY_CIRCUMFLEX},
   //{, KEY_COLON2},
   //{, KEY_KANJI},
   {XK_KP_Equal, KEY_EQUALS_PAD},  /* MacOS X */
   //{, KEY_BACKQUOTE},  /* MacOS X */
   //{, KEY_SEMICOLON},  /* MacOS X */
   //{, KEY_COMMAND},  /* MacOS X */

   {XK_Shift_L, KEY_LSHIFT},
   {XK_Shift_R, KEY_RSHIFT},
   {XK_Control_L, KEY_LCONTROL},
   {XK_Control_R, KEY_RCONTROL},
   {XK_Alt_L, KEY_ALT},

   /* Double mappings. This is a bit of a problem, since you can configure
    * X11 differently to what report for those keys.
    */
   {XK_Alt_R, KEY_ALTGR},
   {XK_ISO_Level3_Shift, KEY_ALTGR},
   {XK_Meta_L, KEY_LWIN},
   {XK_Super_L, KEY_LWIN},
   {XK_Meta_R, KEY_RWIN},
   {XK_Super_R, KEY_RWIN},

   {XK_Menu, KEY_MENU},
   {XK_Scroll_Lock, KEY_SCRLOCK},
   {XK_Num_Lock, KEY_NUMLOCK},
   {XK_Caps_Lock, KEY_CAPSLOCK}
};

/* Table of: Allegro's modifier flag, associated X11 flag, toggle method. */
static int modifier_flags[8][3] = {
   {KB_SHIFT_FLAG, ShiftMask, 0},
   {KB_CAPSLOCK_FLAG, LockMask, 1},
   {KB_CTRL_FLAG, ControlMask, 0},
   {KB_ALT_FLAG, Mod1Mask, 0},
   {KB_NUMLOCK_FLAG, Mod2Mask, 1},
   {KB_SCROLOCK_FLAG, Mod3Mask, 1},
   {KB_LWIN_FLAG | KB_RWIN_FLAG, Mod4Mask, 0}, /* Should we use only one? */
   {KB_MENU_FLAG, Mod5Mask, 0} /* AltGr */
};

/* Table of key names. */
static char AL_CONST *key_names[1 + KEY_MAX];



/* update_shifts
 *  Update Allegro's key_shifts variable, directly from the corresponding
 *  X11 modifiers state.
 */
static void update_shifts(XKeyEvent *event)
{
   int mask = 0;
   int i;

   for (i = 0; i < 8; i++) {
      int j;
      /* This is the state of the modifiers just before the key
       * press/release.
       */
      if (event->state & modifier_flags[i][1])
         mask |= modifier_flags[i][0];

      /* In case a modifier key itself was pressed, we now need to update
       * the above state for Allegro, which wants the state after the
       * press, not before as reported by X.
       */
      for (j = 0; j < xmodmap->max_keypermod; j++) {
         if (event->keycode && event->keycode ==
            xmodmap->modifiermap[i * xmodmap->max_keypermod + j]) {
            if (event->type == KeyPress) {
               /* Modifier key pressed - toggle or set flag. */
               if (modifier_flags[i][2])
                  mask ^= modifier_flags[i][0];
               else
                  mask |= modifier_flags[i][0];
            }
            else if (event->type == KeyRelease) {
               /* Modifier key of non-toggle key released - remove flag. */
               if (!modifier_flags[i][2])
                  mask &= ~modifier_flags[i][0];
            }
         }
      }
   }
   _key_shifts = mask;
}



/* dga2_update_shifts
 *  DGA2 doesn't seem to have a reliable state field. Therefore Allegro must
 *  take care of modifier keys itself.
 */
static void dga2_update_shifts(XKeyEvent *event)
{
   int i, j;

   for (i = 0; i < 8; i++) {
      for (j = 0; j < xmodmap->max_keypermod; j++) {
         if (event->keycode && event->keycode ==
            xmodmap->modifiermap[i * xmodmap->max_keypermod + j]) {
            if (event->type == KeyPress) {
               if (modifier_flags[i][2])
                  _key_shifts ^= modifier_flags[i][0];
               else
                  _key_shifts |= modifier_flags[i][0];
            }
            else if (event->type == KeyRelease) {
               if (!modifier_flags[i][2])
                  _key_shifts &= ~modifier_flags[i][0];
            }
         }
      }
      /* Hack: DGA keys seem to get reported wrong otherwise. */
      if (_key_shifts & modifier_flags[i][0])
         event->state |= modifier_flags[i][1];
   }
}



/* find_unknown_key_assignment
 *  In some cases, X11 doesn't report any KeySym for a key - so the earliest
 *  time we can map it to an Allegro key is when it is first pressed.
 */
static int find_unknown_key_assignment (int i)
{
   int j;

   for (j = 1; j < KEY_MAX; j++) {
      if (!used[j]) {
         AL_CONST char *str;
         _xwin.keycode_to_scancode[i] = j;
         str = XKeysymToString(keysyms[sym_per_key * (i - min_keycode)]);
         if (str)
            key_names[j] = str;
         else
            key_names[j] = _keyboard_common_names[j];
         used[j] = 1;
         break;
      }
   }

   if (j == KEY_MAX) {
      TRACE(PREFIX_E "You have more keys reported by X than Allegro's "
            "maximum of %i keys. Please send a bug report.\n", KEY_MAX);
      _xwin.keycode_to_scancode[i] = 0;
   }

   TRACE(PREFIX_I "Key %i missing:", i);
   for (j = 0; j < sym_per_key; j++) {
      char *sym_str = XKeysymToString(keysyms[sym_per_key * (i - min_keycode) + j]);
      TRACE(" %s", sym_str ? sym_str : "NULL");
   }
   TRACE(" - assigned to %i.\n", _xwin.keycode_to_scancode[i]);

   return _xwin.keycode_to_scancode[i];
}



/* _xwin_keyboard_handler:
 *  Keyboard "interrupt" handler.
 */
void _xwin_keyboard_handler(XKeyEvent *event, int dga2_hack)
{
   int keycode;

   if (!xkeyboard_installed)
      return;

   if (_xwin_keyboard_callback)
      (*_xwin_keyboard_callback)(event->type == KeyPress ? 1 : 0, event->keycode);

   keycode = _xwin.keycode_to_scancode[event->keycode];
   if (keycode == -1)
      keycode = find_unknown_key_assignment(event->keycode);

   if (dga2_hack)
      dga2_update_shifts(event);
   else
      update_shifts(event);

   /* Special case the pause key. */
   if (keycode == KEY_PAUSE) {
      /* Allegro ignore's releasing of the pause key. */
      if (event->type == KeyRelease)
         return;
      if (pause_key) {
         event->type = KeyRelease;
         pause_key = 0;
      }
      else {
         pause_key = 1;
      }
   }

   if (event->type == KeyPress) { /* Key pressed.  */
      int len;
      char buffer[16];
      char buffer2[16];
      int unicode = 0, r = 0;

#if defined (ALLEGRO_XWINDOWS_WITH_XIM) && defined(X_HAVE_UTF8_STRING)
      if (xic) {
         len = Xutf8LookupString(xic, event, buffer, sizeof buffer, NULL, NULL);
      }
      else
#endif
      {
         /* XLookupString is supposed to only use ASCII. */
         len = XLookupString(event, buffer, sizeof buffer, NULL, NULL);
      }
      buffer[len] = '\0';
      uconvert(buffer, U_UTF8, buffer2, U_UNICODE, sizeof buffer2);
      unicode = *(unsigned short *)buffer2;

#ifdef ALLEGRO_XWINDOWS_WITH_XIM
      r = XFilterEvent((XEvent *)event, _xwin.window);
#endif
      if (keycode || unicode) {
         /* If we have a keycode, we want it to go to Allegro immediately, so the
          * key[] array is updated, and the user callbacks are called. OTOH, a key
          * should not be added to the keyboard buffer (parameter -1) if it was
          * filtered out as a compose key, or if it is a modifier key.
          */
         if (r || keycode >= KEY_MODIFIERS)
            unicode = -1;
         else {
            /* Historically, Allegro expects to get only the scancode when Alt is
             * held down.
             */
            if (_key_shifts & KB_ALT_FLAG)
               unicode = 0;
         }

         _handle_key_press(unicode, keycode);

         /* Detect Ctrl-Alt-End. */
         if (keycode == KEY_END && (_key_shifts & KB_CTRL_FLAG) &&
            (_key_shifts & KB_ALT_FLAG) && (three_finger_flag))
         {
         #ifndef ALLEGRO_HAVE_LIBPTHREAD
            if (_unix_bg_man == &_bg_man_sigalrm) {
               _sigalrm_request_abort();
            }
            else
         #endif
            {
               TRACE(PREFIX_W "Three finger combo detected. SIGTERMing "
                     "pid %d\n", main_pid);
               kill(main_pid, SIGTERM);
            }
         }
      }
   }
   else { /* Key release. */
      _handle_key_release(keycode);
   }
}



/* _xwin_keyboard_focus_handler:
 *  Handles switching of X keyboard focus.
 */
void _xwin_keyboard_focus_handler(XFocusChangeEvent *event)
{
   /* Simulate release of all keys on focus out. */
   if (event->type == FocusOut) {
      int i;

      for (i = 0; i < KEY_MAX; i++) {
         if (key[i])
            _handle_key_release(i);
      }
   }
}



/* find_allegro_key
 *  Search the translation table for the Allegro key corresponding to the
 *  given KeySym.
 */
static int find_allegro_key(KeySym sym)
{
   int i;
   int n = sizeof translation_table / sizeof *translation_table;

   for (i = 0; i < n; i++) {
      if (translation_table[i].keysym == sym)
         return translation_table[i].allegro_key;
   }
   return 0;
}



/* scancode_to_name:
 *  Converts the given scancode to a description of the key.
 */
static AL_CONST char *x_scancode_to_name(int scancode)
{
   ASSERT(scancode >= 0 && scancode < KEY_MAX);

   return key_names[scancode];
}



/* x_get_keyboard_mapping:
 *  Generate a mapping from X11 keycodes to Allegro KEY_* codes. We have
 *  two goals: Every keypress should be mapped to a distinct Allegro KEY_*
 *  code. And we want the KEY_* codes to match the pressed
 *  key to some extent. To do the latter, the X11 KeySyms produced by a key
 *  are examined. If a match is found in the table above, the mapping is
 *  added to the mapping table. If no known KeySym is found for a key (or
 *  the same KeySym is found for more keys) - the remaining keys are
 *  distributed arbitrarily to the remaining KEY_* codes.
 *
 *  In a future version, this could be simplified by mapping *all* the X11
 *  KeySyms to KEY_* codes.
 */
void _xwin_get_keyboard_mapping(void)
{
   int i;
   int count;
   int missing = 0;

   memset(used, 0, sizeof used);
   memset(_xwin.keycode_to_scancode, 0, sizeof _xwin.keycode_to_scancode);

   XLOCK();

   XDisplayKeycodes(_xwin.display, &min_keycode, &max_keycode);
   count = 1 + max_keycode - min_keycode;

   if (keysyms) {
      XFree(keysyms);
   }
   keysyms = XGetKeyboardMapping(_xwin.display, min_keycode,
      count, &sym_per_key);

   TRACE (PREFIX_I "%i keys, %i symbols per key.\n", count, sym_per_key);

   missing = 0;

   for (i = min_keycode; i <= max_keycode; i++) {
      KeySym sym = keysyms[sym_per_key * (i - min_keycode)];
      KeySym sym2 =  keysyms[sym_per_key * (i - min_keycode) + 1];
      char *sym_str, *sym2_str;
      int allegro_key = 0;

      sym_str = XKeysymToString(sym);
      sym2_str = XKeysymToString(sym2);

      TRACE (PREFIX_I "key [%i: %s %s]", i, sym_str ? sym_str : "NULL", sym2_str ?
         sym2_str : "NULL");

      /* Hack for French keyboards, to correctly map KEY_0 to KEY_9. */
      if (sym2 >= XK_0 && sym2 <= XK_9) {
         allegro_key = find_allegro_key(sym2);
      }

      if (!allegro_key) {
         if (sym != NoSymbol) {
            allegro_key = find_allegro_key(sym);

            if (allegro_key == 0) {
               missing++;
               TRACE (" defering.\n");
            }
         }
         else {
            /* No KeySym for this key - ignore it. */
            _xwin.keycode_to_scancode[i] = -1;
            TRACE (" not assigned.\n");
         }
      }

      if (allegro_key) {
         if (used[allegro_key]) {
            TRACE(" *double*");
         }
         _xwin.keycode_to_scancode[i] = allegro_key;
         key_names[allegro_key] =
            XKeysymToString(keysyms[sym_per_key * (i - min_keycode)]);
         used[allegro_key] = 1;
         TRACE(" assigned to %i.\n", allegro_key);
      }
   }

   if (missing) {
      /* The keys still not assigned are just assigned arbitrarily now. */
      for (i = min_keycode; i <= max_keycode; i++) {
         if (_xwin.keycode_to_scancode[i] == 0) {
            find_unknown_key_assignment(i);
         }
      }
   }

   if (xmodmap)
      XFreeModifiermap(xmodmap);
   xmodmap = XGetModifierMapping(_xwin.display);
   for (i = 0; i < 8; i++) {
      int j;

      TRACE (PREFIX_I "Modifier %d:", i + 1);
      for (j = 0; j < xmodmap->max_keypermod; j++) {
         KeySym sym = XKeycodeToKeysym(_xwin.display,
            xmodmap->modifiermap[i * xmodmap->max_keypermod + j], 0);
         char *sym_str = XKeysymToString(sym);
         TRACE(" %s", sym_str ? sym_str : "NULL");
      }
      TRACE("\n");
   }

   /* The [xkeymap] section can be useful, e.g. if trying to play a
    * game which has X and Y hardcoded as KEY_X and KEY_Y to mean
    * left/right movement, but on the X11 keyboard X and Y are far apart.
    * For normal use, a user never should have to touch [xkeymap] anymore
    * though, and proper written programs will not hardcode such mappings.
    */
   {
      char *section, *option_format;
      char option[128], tmp1[128], tmp2[128];

      section = uconvert_ascii("xkeymap", tmp1);
      option_format = uconvert_ascii("keycode%d", tmp2);

      for (i = min_keycode; i <= max_keycode; i++) {
         int scancode;

         uszprintf(option, sizeof(option), option_format, i);
         scancode = get_config_int(section, option, -1);
         if (scancode > 0) {
            _xwin.keycode_to_scancode[i] = scancode;
            TRACE(PREFIX_I "User override: KeySym %i assigned to %i.\n", i, scancode);
         }
      }
   }

   XUNLOCK();
}



/* x_set_leds:
 *  Update the keyboard LEDs.
 */
static void x_set_leds(int leds)
{
   XKeyboardControl values;

   if (!xkeyboard_installed)
      return;

   XLOCK();

   values.led = 1;
   values.led_mode = leds & KB_NUMLOCK_FLAG ? LedModeOn : LedModeOff;
   XChangeKeyboardControl(_xwin.display, KBLed | KBLedMode, &values);

   values.led = 2;
   values.led_mode = leds & KB_CAPSLOCK_FLAG ? LedModeOn : LedModeOff;
   XChangeKeyboardControl(_xwin.display, KBLed | KBLedMode, &values);

   values.led = 3;
   values.led_mode = leds & KB_SCROLOCK_FLAG ? LedModeOn : LedModeOff;
   XChangeKeyboardControl(_xwin.display, KBLed | KBLedMode, &values);

   XUNLOCK();
}



/* x_keyboard_init
 *  Initialise the X11 keyboard driver.
 */
static int x_keyboard_init(void)
{
#ifdef ALLEGRO_XWINDOWS_WITH_XIM
   XIMStyles *xim_styles;
   XIMStyle xim_style = 0;
   char *imvalret;
   int i;
#endif

   if (xkeyboard_installed)
      return 0;

   main_pid = getpid();

   memcpy(key_names, _keyboard_common_names, sizeof key_names);

   XLOCK ();

#ifdef ALLEGRO_XWINDOWS_WITH_XIM
   /* Otherwise we are restricted to ISO-8859-1 characters. */
/* [dacap] Don't call setlocale()
   if (setlocale(LC_ALL, "") == NULL) {
      TRACE(PREFIX_W "Could not set default locale.\n");
   }
*/

/* TODO: is this needed?
   modifiers = XSetLocaleModifiers("@im=none");
   if (modifiers == NULL) {
      TRACE(PREFIX_W "XSetLocaleModifiers failed.\n");
   }
*/
   xim = XOpenIM(_xwin.display, NULL, NULL, NULL);
   if (xim == NULL) {
      TRACE(PREFIX_W "XOpenIM failed.\n");
   }

   if (xim) {
      imvalret = XGetIMValues(xim, XNQueryInputStyle, &xim_styles, NULL);
      if (imvalret != NULL || xim_styles == NULL) {
         TRACE(PREFIX_W "Input method doesn't support any styles.\n");
      }

      if (xim_styles) {
         xim_style = 0;
         for (i = 0;  i < xim_styles->count_styles;  i++) {
            if (xim_styles->supported_styles[i] ==
               (XIMPreeditNothing | XIMStatusNothing)) {
               xim_style = xim_styles->supported_styles[i];
               break;
            }
         }

         if (xim_style == 0) {
            TRACE (PREFIX_W "Input method doesn't support the style we support.\n");
         }
         XFree(xim_styles);
      }
   }

   if (xim && xim_style) {
      xic = XCreateIC(xim,
                       XNInputStyle, xim_style,
                       XNClientWindow, _xwin.window,
                       XNFocusWindow, _xwin.window,
                       NULL);

      if (xic == NULL) {
         TRACE (PREFIX_W "XCreateIC failed.\n");
      }
   }
#endif

   _xwin_get_keyboard_mapping();

   XUNLOCK();

   xkeyboard_installed = 1;

   return 0;
}



/* x_keyboard_exit
 *  Shut down the X11 keyboard driver.
 */
static void x_keyboard_exit(void)
{
   if (!xkeyboard_installed)
      return;
   xkeyboard_installed = 0;

   XLOCK();

#ifdef ALLEGRO_XWINDOWS_WITH_XIM

   if (xic) {
      XDestroyIC(xic);
      xic = NULL;
   }

   if (xim) {
      XCloseIM(xim);
      xim = NULL;
   }

#endif

   if (xmodmap) {
      XFreeModifiermap(xmodmap);
      xmodmap = NULL;
   }

   if (keysyms) {
      XFree(keysyms);
      keysyms = NULL;
   }

   XUNLOCK();
}



static KEYBOARD_DRIVER keyboard_x =
{
   KEYBOARD_XWINDOWS,
   "X11 keyboard",
   "X11 keyboard",
   "X11 keyboard",
   FALSE,
   x_keyboard_init,
   x_keyboard_exit,
   NULL,   // AL_METHOD(void, poll, (void));
   x_set_leds,
   NULL,   // AL_METHOD(void, set_rate, (int delay, int rate));
   NULL,   // AL_METHOD(void, wait_for_input, (void));
   NULL,   // AL_METHOD(void, stop_waiting_for_input, (void));
   NULL,   // AL_METHOD(int,  scancode_to_ascii, (int scancode));
   x_scancode_to_name
};

/* list the available drivers */
_DRIVER_INFO _xwin_keyboard_driver_list[] =
{
   {  KEYBOARD_XWINDOWS, &keyboard_x,    TRUE  },
   {  0,                 NULL,           0     }
};
