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
 *      Keyboard input routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"



#define PREFIX_I                "al-keyboard INFO: "
#define PREFIX_W                "al-keyboard WARNING: "
#define PREFIX_E                "al-keyboard ERROR: "


KEYBOARD_DRIVER *keyboard_driver = NULL;     /* the active driver */

int _keyboard_installed = FALSE;

static int keyboard_polled = FALSE;          /* are we in polling mode? */

int three_finger_flag = TRUE;                /* mode flags */
int key_led_flag = TRUE;

volatile char key[KEY_MAX];                  /* key pressed flags */
volatile char _key[KEY_MAX];

volatile int key_shifts = 0;                 /* current shift state */
volatile int _key_shifts = 0;

int (*keyboard_callback)(int key) = NULL;    /* callback functions */
int (*keyboard_ucallback)(int key, int *scancode) = NULL;
void (*keyboard_lowlevel_callback)(int scancode) = NULL;

static int (*keypressed_hook)(void) = NULL;  /* hook functions */
static int (*readkey_hook)(void) = NULL;

static int waiting_for_input = FALSE;        /* idle flag */

static int repeat_delay = 250;               /* auto key repeat */
static int repeat_rate = 33;
static int repeat_key = -1;
static int repeat_scan = -1;

static int rate_changed = FALSE;

/* Provide a default ASCII mapping for the most common keys. Keys whose
 * mapping changes dependind on the layout aren't listed - it's up to
 * the keyboard driver to do that. The reason for this it portability:
 * With an English keyboard, it makes sense to just list the KEY_* names,
 * but not with other keyboards. Ideally, every keyboard driver would
 * be able to fill in all the keys - but where it doesn't, this table
 * is a good compromise.
 */
static int common_ascii[KEY_MAX] =
{
   0,   'a', 'b', 'c', 'd', 'e', 'f', 'g',
   'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
   'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
   'x', 'y', 'z', '0', '1', '2', '3', '4',
   '5', '6', '7', '8', '9', '0', '1', '2',
   '3', '4', '5', '6', '7', '8', '9', 0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   27,  0,   0,   0,   0,
   9,   0,   0,   13,  0,   0,   0,   0,
   0,   0,   0,   ' ', 0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   '/', '*',
   '-', '+', 0,  13,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   '=',
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0
};

/* The same as above, but with descriptive names. Again, only keys which are
 * likely to be found on every keyboard are named. All keyboard drivers should
 * provide their own implementation though, especially if they use positional
 * mapping. */
AL_CONST char *_keyboard_common_names[KEY_MAX] =
{
   "(none)",     "A",          "B",          "C",
   "D",          "E",          "F",          "G",
   "H",          "I",          "J",          "K",
   "L",          "M",          "N",          "O",
   "P",          "Q",          "R",          "S",
   "T",          "U",          "V",          "W",
   "X",          "Y",          "Z",          "0",
   "1",          "2",          "3",          "4",
   "5",          "6",          "7",          "8",
   "9",          "0 PAD",      "1 PAD",      "2 PAD",
   "3 PAD",      "4 PAD",      "5 PAD",      "6 PAD",
   "7 PAD",      "8 PAD",      "9 PAD",      "F1",
   "F2",         "F3",         "F4",         "F5",
   "F6",         "F7",         "F8",         "F9",
   "F10",        "F11",        "F12",        "ESC",
   "KEY60",      "KEY61",      "KEY62",      "BACKSPACE",
   "TAB",        "KEY65",      "KEY66",      "ENTER",
   "KEY68",      "KEY69",      "BACKSLASH",  "KEY71",
   "KEY72",      "KEY73",      "KEY74",      "SPACE",
   "INSERT",     "DEL",        "HOME",       "END",
   "PGUP",       "PGDN",       "LEFT",       "RIGHT",
   "UP",         "DOWN",       "/ PAD",      "* PAD",
   "- PAD",      "+ PAD",      "DEL PAD",    "ENTER PAD",
   "PRINT",      "PAUSE",      "KEY94",      "KEY95",
   "KEY96",      "KEY97",      "KEY98",      "KEY99",
   "KEY100",     "KEY101",     "KEY102",     "= PAD",
   "KEY104",     "KEY105",     "KEY106",     "KEY107",
   "KEY108",     "KEY109",     "KEY110",     "KEY111",
   "KEY112",     "KEY113",     "KEY114",     "LSHIFT",
   "RSHIFT",     "LCONTROL",   "RCONTROL",   "ALT",
   "ALTGR",      "LWIN",       "RWIN",       "MENU",
   "SCRLOCK",    "NUMLOCK",    "CAPSLOCK"
};

#define KEY_BUFFER_SIZE    64                /* character ring buffer */

typedef struct KEY_BUFFER
{
   volatile int lock;
   volatile int start;
   volatile int end;
   volatile int key[KEY_BUFFER_SIZE];
   volatile unsigned char scancode[KEY_BUFFER_SIZE];
} KEY_BUFFER;

static volatile KEY_BUFFER key_buffer;
static volatile KEY_BUFFER _key_buffer;



/* add_key:
 *  Helper function to add a keypress to a buffer.
 */
static INLINE void add_key(volatile KEY_BUFFER *buffer, int key, int scancode)
{
   int c, d;

   if (buffer == &key_buffer) {
      if (keyboard_ucallback) {
         key = keyboard_ucallback(key, &scancode);
         if ((!key) && (!scancode))
            return;
      }
      else if (keyboard_callback) {
         c = ((key <= 0xFF) ? key : '^') | (scancode << 8);
         d = keyboard_callback(c);

         if (!d)
            return;

         if (d != c) {
            key = (d & 0xFF);
            scancode = (d >> 8);
         }
      }
   }

   buffer->lock++;

   if (buffer->lock != 1) {
      buffer->lock--;
      return;
   }

   if ((waiting_for_input) && (keyboard_driver) && (keyboard_driver->stop_waiting_for_input))
      keyboard_driver->stop_waiting_for_input();

   if (buffer->end < KEY_BUFFER_SIZE-1)
      c = buffer->end+1;
   else
      c = 0;

   if (c != buffer->start) {
      buffer->key[buffer->end] = key;
      buffer->scancode[buffer->end] = scancode;
      buffer->end = c;
   }

   buffer->lock--;
}



/* clear_keybuf:
 *  Clears the keyboard buffer.
 */
void clear_keybuf(void)
{
   if (keyboard_polled)
      poll_keyboard();

   key_buffer.lock++;
   _key_buffer.lock++;

   key_buffer.start = key_buffer.end = 0;
   _key_buffer.start = _key_buffer.end = 0;

   key_buffer.lock--;
   _key_buffer.lock--;

   if ((keypressed_hook) && (readkey_hook))
      while (keypressed_hook())
         readkey_hook();
}



/* clear_key:
 *  Helper function to clear the key[] array.
 */
static void clear_key(void)
{
   int c;

   for (c=0; c<KEY_MAX; c++) {
      key[c] = 0;
      _key[c] = 0;
   }
}



/* keypressed:
 *  Returns TRUE if there are keypresses waiting in the keyboard buffer.
 */
int keypressed(void)
{
   if (keyboard_polled)
      poll_keyboard();

   if (key_buffer.start == key_buffer.end) {
      if (keypressed_hook)
         return keypressed_hook();
      else
         return FALSE;
   }
   else
      return TRUE;
}



/* readkey:
 *  Returns the next character code from the keyboard buffer. If the
 *  buffer is empty, it waits until a key is pressed. The low byte of
 *  the return value contains the ASCII code of the key, and the high
 *  byte the scan code.
 */
int readkey(void)
{
   int key, scancode;

   key = ureadkey(&scancode);

   return ((key <= 0xFF) ? key : '^') | (scancode << 8);
}



/* ureadkey:
 *  Returns the next character code from the keyboard buffer. If the
 *  buffer is empty, it waits until a key is pressed. The return value
 *  contains the Unicode value of the key, and the scancode parameter
 *  is set to the scancode.
 */
int ureadkey(int *scancode)
{
   int c;

   if ((!keyboard_driver) && (!readkey_hook)) {
      if (scancode)
         *scancode = 0;
      return 0;
   }

   if ((readkey_hook) && (key_buffer.start == key_buffer.end)) {
      c = readkey_hook();
      if (scancode)
         *scancode = (c >> 8);
      return (c & 0xFF);
   }

   while (key_buffer.start == key_buffer.end) {
      if ((keyboard_driver) && (keyboard_driver->wait_for_input)) {
         waiting_for_input = TRUE;
         keyboard_driver->wait_for_input();
         waiting_for_input = FALSE;
      }

      if (keyboard_polled)
         poll_keyboard();

      rest(1);
   }

   c = key_buffer.key[key_buffer.start];

   if (scancode)
      *scancode = key_buffer.scancode[key_buffer.start];

   if (key_buffer.start < KEY_BUFFER_SIZE-1)
      key_buffer.start++;
   else
      key_buffer.start = 0;

   return c;
}



/* simulate_keypress:
 *  Pushes a key into the keyboard buffer, as if it has just been pressed.
 */
void simulate_keypress(int key)
{
   add_key(&key_buffer, key&0xFF, key>>8);
}



/* simulate_ukeypress:
 *  Pushes a key into the keyboard buffer, as if it has just been pressed.
 */
void simulate_ukeypress(int key, int scancode)
{
   add_key(&key_buffer, key, scancode);
}



/* set_leds:
 *  Overrides the state of the keyboard LED indicators.
 *  Set to -1 to return to default behavior.
 */
void set_leds(int leds)
{
   if (leds < 0) {
      key_led_flag = TRUE;
      leds = key_shifts;
   }
   else
      key_led_flag = FALSE;

   if ((keyboard_driver) && (keyboard_driver->set_leds))
      keyboard_driver->set_leds(leds);
}



/* set_keyboard_rate:
 *  Sets the keyboard repeat rate. Times are given in milliseconds.
 *  Passing zero times will disable the key repeat.
 */
void set_keyboard_rate(int delay, int repeat)
{
   repeat_delay = delay;
   repeat_rate = repeat;

   if ((repeat_delay) && (keyboard_driver) && (keyboard_driver->set_rate)) {
      keyboard_driver->set_rate(delay, repeat);
      rate_changed = TRUE;
   }
}



/* repeat_timer:
 *  Timer callback for doing automatic key repeats.
 */
static void repeat_timer(void)
{
   if (keyboard_driver)
      _handle_key_press(repeat_key, repeat_scan);

   install_int(repeat_timer, repeat_rate);
}

END_OF_STATIC_FUNCTION(repeat_timer);



/* install_keyboard_hooks:
 *  You should only use this function if you *aren't* using the rest of the
 *  keyboard handler. It can be called in the place of install_keyboard(),
 *  and lets you provide callback routines to detect and read keypresses,
 *  which will be used by the main keypressed() and readkey() functions. This
 *  can be useful if you want to use Allegro's GUI code with a custom
 *  keyboard handler, as it provides a way for the GUI to access keyboard
 *  input from your own code.
 */
void install_keyboard_hooks(int (*keypressed)(void), int (*readkey)(void))
{
   key_buffer.lock = _key_buffer.lock = 0;

   clear_keybuf();
   clear_key();

   keypressed_hook = keypressed;
   readkey_hook = readkey;
}



/* update_shifts:
 *  Helper function to update the key_shifts variable and LED state.
 */
static INLINE void update_shifts(void)
{
   #define LED_FLAGS  (KB_SCROLOCK_FLAG | KB_NUMLOCK_FLAG | KB_CAPSLOCK_FLAG)

   if (key_shifts != _key_shifts) {
      if ((keyboard_driver->set_leds) && (key_led_flag) &&
          ((key_shifts ^ _key_shifts) & LED_FLAGS))
         keyboard_driver->set_leds(_key_shifts);

      key_shifts = _key_shifts;
   }
}



/* _handle_key_press:
 *  Called by the hardware driver to tell us when a key has been pressed.
 */
void _handle_key_press(int keycode, int scancode)
{
   ASSERT(keyboard_driver);

   if (scancode < 0 || scancode >= KEY_MAX) {
      TRACE(PREFIX_I "Scancode out of range %d pressed\n", scancode);
      return;
   }

   if ((keyboard_driver->poll) || (!keyboard_polled)) {
      /* process immediately */
      if (scancode > 0) {
         if ((!repeat_delay) && (key[scancode]))
            return;

         key[scancode] = -1;

         if (keyboard_lowlevel_callback)
            keyboard_lowlevel_callback(scancode);
      }

      /* e.g. for F1, keycode=0, and scancode=KEY_F1 */
      if (keycode >= 0)
         add_key(&key_buffer, keycode, scancode);

      update_shifts();
   }
   else {
      /* deal with this during the next poll_keyboard() */
      if (scancode > 0) {
         if ((!repeat_delay) && (_key[scancode]))
            return;

         _key[scancode] = -1;
      }

      if (keycode >= 0)
         add_key(&_key_buffer, keycode, scancode);
   }

   /* autorepeat? */
   if ((keyboard_driver->autorepeat) && (repeat_delay) &&
       (keycode >= 0) && (scancode > 0) && (scancode != KEY_PAUSE) &&
       ((keycode != repeat_key) || (scancode != repeat_scan))) {
      repeat_key = keycode;
      repeat_scan = scancode;
      remove_int(repeat_timer);
      install_int(repeat_timer, repeat_delay);
   }
}

END_OF_FUNCTION(_handle_key_press);



/* _handle_key_release:
 *  Called by the hardware driver to tell us when a key has been released.
 */
void _handle_key_release(int scancode)
{
   /* turn off autorepeat for the previous key */
   if (repeat_scan == scancode) {
      remove_int(repeat_timer);
      repeat_key = -1;
      repeat_scan = -1;
   }

   if (scancode < 0 || scancode >= KEY_MAX) {
      TRACE(PREFIX_I "Scancode out of range %d released\n", scancode);
      return;
   }

   if ((keyboard_driver->poll) || (!keyboard_polled)) {
      /* process immediately */
      key[scancode] = 0;

      if (keyboard_lowlevel_callback)
         keyboard_lowlevel_callback(scancode | 0x80);

      update_shifts();
   }
   else {
      /* deal with this during the next poll_keyboard() */
      _key[scancode] = 0;
   }
}

END_OF_FUNCTION(_handle_key_release);



/* poll_keyboard:
 *  Polls the current keyboard state, and updates the user-visible
 *  information accordingly. On some drivers this is actually required
 *  to get the input, while on others it is only present to keep
 *  compatibility with systems that do need it. So that people can test
 *  their polling code even on platforms that don't strictly require it,
 *  after this function has been called once, the entire system will
 *  switch into polling mode and will no longer operate asynchronously
 *  even if the driver actually does support that.
 */
int poll_keyboard(void)
{
   int i;

   if (!keyboard_driver)
      return -1;

   if (keyboard_driver->poll) {
      /* poll the driver */
      keyboard_driver->poll();
   }
   else if (!keyboard_polled) {
      /* switch into polling emulation mode */
      for (i=0; i<KEY_MAX; i++)
         _key[i] = key[i];

      keyboard_polled = TRUE;
   }
   else {
      /* update the real keyboard variables with stored input */
      for (i=0; i<KEY_MAX; i++) {
         if (key[i] != _key[i]) {
            key[i] = _key[i];

            if (keyboard_lowlevel_callback)
               keyboard_lowlevel_callback((key[i]) ? i : (i | 0x80));
         }
      }

      while (_key_buffer.start != _key_buffer.end) {
         add_key(&key_buffer, _key_buffer.key[_key_buffer.start],
                              _key_buffer.scancode[_key_buffer.start]);

         if (_key_buffer.start < KEY_BUFFER_SIZE-1)
            _key_buffer.start++;
         else
            _key_buffer.start = 0;
      }

      update_shifts();
   }

   return 0;
}



/* keyboard_needs_poll:
 *  Checks whether the current driver uses polling.
 */
int keyboard_needs_poll(void)
{
   return keyboard_polled;
}



/* scancode_to_ascii:
 *  Converts the given scancode to an ASCII character for that key --
 *  that is the unshifted uncapslocked result of pressing the key, or
 *  0 if the key isn't a character-generating key or the lookup can't
 *  be done.
 */
int scancode_to_ascii(int scancode)
{
   ASSERT (scancode >= 0 && scancode < KEY_MAX);
   if (keyboard_driver->scancode_to_ascii)
      return keyboard_driver->scancode_to_ascii(scancode);
   else
      return common_ascii[scancode];
}



/* scancode_to_name:
 *  Converts the given scancode to a description of the key.
 */
AL_CONST char *scancode_to_name(int scancode)
{
   AL_CONST char *name = NULL;

   ASSERT(keyboard_driver);
   ASSERT((scancode >= 0) && (scancode < KEY_MAX));

   if (keyboard_driver->scancode_to_name)
      name = keyboard_driver->scancode_to_name(scancode);

   if (!name)
      name = _keyboard_common_names[scancode];

   ASSERT(name);

   return name;
}



/* install_keyboard:
 *  Installs Allegro's keyboard handler. You must call this before using
 *  any of the keyboard input routines. Returns -1 on failure.
 */
int install_keyboard(void)
{
   _DRIVER_INFO *driver_list;
   int i;

   if (keyboard_driver)
      return 0;

   LOCK_VARIABLE(keyboard_driver);
   LOCK_VARIABLE(keyboard_polled);
   LOCK_VARIABLE(key);
   LOCK_VARIABLE(_key);
   LOCK_VARIABLE(key_shifts);
   LOCK_VARIABLE(_key_shifts);
   LOCK_VARIABLE(key_buffer);
   LOCK_VARIABLE(_key_buffer);
   LOCK_VARIABLE(three_finger_flag);
   LOCK_VARIABLE(key_led_flag);
   LOCK_VARIABLE(keyboard_callback);
   LOCK_VARIABLE(keyboard_ucallback);
   LOCK_VARIABLE(keyboard_lowlevel_callback);
   LOCK_VARIABLE(waiting_for_input);
   LOCK_VARIABLE(repeat_delay);
   LOCK_VARIABLE(repeat_rate);
   LOCK_VARIABLE(repeat_key);
   LOCK_VARIABLE(repeat_scan);
   LOCK_FUNCTION(_handle_key_press);
   LOCK_FUNCTION(_handle_key_release);
   LOCK_FUNCTION(repeat_timer);

   key_buffer.lock = _key_buffer.lock = 0;

   clear_keybuf();
   clear_key();

   if (system_driver->keyboard_drivers)
      driver_list = system_driver->keyboard_drivers();
   else
      driver_list = _keyboard_driver_list;

   for (i=0; driver_list[i].driver; i++) {
      keyboard_driver = driver_list[i].driver;
      keyboard_driver->name = keyboard_driver->desc = get_config_text(keyboard_driver->ascii_name);
      if (keyboard_driver->init() == 0)
         break;
   }

   if (!driver_list[i].driver) {
      keyboard_driver = NULL;
      return -1;
   }

   keyboard_polled = (keyboard_driver->poll) ? TRUE : FALSE;

   set_leds(-1);

   _add_exit_func(remove_keyboard, "remove_keyboard");
   _keyboard_installed = TRUE;

   if ((keyboard_driver->autorepeat) && (!_timer_installed))
      install_timer();

   update_shifts();

   return 0;
}



/* remove_keyboard:
 *  Removes the keyboard handler. You don't normally need to call this,
 *  because allegro_exit() will do it for you.
 */
void remove_keyboard(void)
{
   if (!keyboard_driver)
      return;

   set_leds(-1);

   if (rate_changed) {
      set_keyboard_rate(250, 33);
      rate_changed = FALSE;
   }

   keyboard_driver->exit();
   keyboard_driver = NULL;

   if (repeat_key >= 0) {
      remove_int(repeat_timer);
      repeat_key = -1;
      repeat_scan = -1;
   }

   _keyboard_installed = FALSE;

   keyboard_polled = FALSE;

   clear_keybuf();
   clear_key();

   key_shifts = _key_shifts = 0;

   _remove_exit_func(remove_keyboard);
}
