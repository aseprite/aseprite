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
 *      Linux console keyboard module.
 *
 *      By Marek Habersack, modified by George Foot.
 *
 *      See readme.txt for copyright information.
 */


/* Make sure we don't #define our own KEY_ macros as linux has its
 * own.  Instead, we use the __allegro_KEY_ constants in this file.
 */
#define ALLEGRO_NO_KEY_DEFINES

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"
#include "linalleg.h"

#define PREFIX_I                "al-ckey INFO: "
#define PREFIX_W                "al-ckey WARNING: "
#define PREFIX_E                "al-ckey ERROR: "

static KEYBOARD_DRIVER keydrv_linux_console;



/* list the available drivers */
_DRIVER_INFO _linux_keyboard_driver_list[] =
{
   {  KEYDRV_LINUX,   &keydrv_linux_console, TRUE  },
   {  0,               NULL,                 0     }
};


static int  linux_key_init (void);
static void linux_key_exit (void);
static void linux_set_leds (int leds);
static void linux_set_rate (int delay, int rate);
static int  linux_scancode_to_ascii (int scancode);

static KEYBOARD_DRIVER keydrv_linux_console =
{
   KEYDRV_LINUX,
   empty_string,
   empty_string,
   "Linux console keyboard",
   FALSE,
   linux_key_init,
   linux_key_exit,
   NULL,
   linux_set_leds,
   linux_set_rate,
   NULL, NULL,
   linux_scancode_to_ascii,
   NULL /* scancode_to_name */
};

static int update_keyboard (void);
static void resume_keyboard (void);
static void suspend_keyboard (void);

static STD_DRIVER std_keyboard =
{
   STD_KBD,
   update_keyboard,
   resume_keyboard,
   suspend_keyboard,
   -1   /* fd -- filled in later */
};


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <linux/kd.h>
#include <linux/keyboard.h>
#include <linux/vt.h>


static int startup_kbmode;
static int resume_count;
static int main_pid;


#define KB_MODIFIERS    (__allegro_KB_SHIFT_FLAG | __allegro_KB_CTRL_FLAG | __allegro_KB_ALT_FLAG | \
			 __allegro_KB_LWIN_FLAG | __allegro_KB_RWIN_FLAG | __allegro_KB_MENU_FLAG)
#define KB_LED_FLAGS    (__allegro_KB_SCROLOCK_FLAG | __allegro_KB_NUMLOCK_FLAG | __allegro_KB_CAPSLOCK_FLAG)


/* lookup table for converting kernel scancodes into Allegro format */
static unsigned char kernel_to_mycode[128] =
{
   /* 0x00 */  0,			 __allegro_KEY_ESC,        __allegro_KEY_1,          __allegro_KEY_2,
   /* 0x04 */  __allegro_KEY_3,          __allegro_KEY_4,          __allegro_KEY_5,          __allegro_KEY_6,
   /* 0x08 */  __allegro_KEY_7,          __allegro_KEY_8,          __allegro_KEY_9,          __allegro_KEY_0,
   /* 0x0C */  __allegro_KEY_MINUS,      __allegro_KEY_EQUALS,     __allegro_KEY_BACKSPACE,  __allegro_KEY_TAB,
   /* 0x10 */  __allegro_KEY_Q,          __allegro_KEY_W,          __allegro_KEY_E,          __allegro_KEY_R,
   /* 0x14 */  __allegro_KEY_T,          __allegro_KEY_Y,          __allegro_KEY_U,          __allegro_KEY_I,
   /* 0x18 */  __allegro_KEY_O,          __allegro_KEY_P,          __allegro_KEY_OPENBRACE,  __allegro_KEY_CLOSEBRACE,
   /* 0x1C */  __allegro_KEY_ENTER,      __allegro_KEY_LCONTROL,   __allegro_KEY_A,          __allegro_KEY_S,
   /* 0x20 */  __allegro_KEY_D,          __allegro_KEY_F,          __allegro_KEY_G,          __allegro_KEY_H,
   /* 0x24 */  __allegro_KEY_J,          __allegro_KEY_K,          __allegro_KEY_L,          __allegro_KEY_COLON,
   /* 0x28 */  __allegro_KEY_QUOTE,      __allegro_KEY_TILDE,      __allegro_KEY_LSHIFT,     __allegro_KEY_BACKSLASH,
   /* 0x2C */  __allegro_KEY_Z,          __allegro_KEY_X,          __allegro_KEY_C,          __allegro_KEY_V,
   /* 0x30 */  __allegro_KEY_B,          __allegro_KEY_N,          __allegro_KEY_M,          __allegro_KEY_COMMA,
   /* 0x34 */  __allegro_KEY_STOP,       __allegro_KEY_SLASH,      __allegro_KEY_RSHIFT,     __allegro_KEY_ASTERISK,
   /* 0x38 */  __allegro_KEY_ALT,        __allegro_KEY_SPACE,      __allegro_KEY_CAPSLOCK,   __allegro_KEY_F1,
   /* 0x3C */  __allegro_KEY_F2,         __allegro_KEY_F3,         __allegro_KEY_F4,         __allegro_KEY_F5,
   /* 0x40 */  __allegro_KEY_F6,         __allegro_KEY_F7,         __allegro_KEY_F8,         __allegro_KEY_F9,
   /* 0x44 */  __allegro_KEY_F10,        __allegro_KEY_NUMLOCK,    __allegro_KEY_SCRLOCK,    __allegro_KEY_7_PAD,
   /* 0x48 */  __allegro_KEY_8_PAD,      __allegro_KEY_9_PAD,      __allegro_KEY_MINUS_PAD,  __allegro_KEY_4_PAD,
   /* 0x4C */  __allegro_KEY_5_PAD,      __allegro_KEY_6_PAD,      __allegro_KEY_PLUS_PAD,   __allegro_KEY_1_PAD,
   /* 0x50 */  __allegro_KEY_2_PAD,      __allegro_KEY_3_PAD,      __allegro_KEY_0_PAD,      __allegro_KEY_DEL_PAD,
   /* 0x54 */  0,			 0,			   __allegro_KEY_BACKSLASH2, __allegro_KEY_F11,
   /* 0x58 */  __allegro_KEY_F12,        0,			   0,			     0,
   /* 0x5C */  0,			 0,			   0,			     0,
   /* 0x60 */  __allegro_KEY_ENTER_PAD,  __allegro_KEY_RCONTROL,   __allegro_KEY_SLASH_PAD,  __allegro_KEY_PRTSCR,
   /* 0x64 */  __allegro_KEY_ALTGR,      0,			   __allegro_KEY_HOME,       __allegro_KEY_UP,
   /* 0x68 */  __allegro_KEY_PGUP,       __allegro_KEY_LEFT,       __allegro_KEY_RIGHT,      __allegro_KEY_END,
   /* 0x6C */  __allegro_KEY_DOWN,       __allegro_KEY_PGDN,       __allegro_KEY_INSERT,     __allegro_KEY_DEL,
   /* 0x70 */  0,			 0,			   0,			     0,
   /* 0x74 */  0,			 0,			   0,			     __allegro_KEY_PAUSE,
   /* 0x78 */  0,			 0,			   0,			     0,
   /* 0x7C */  0,			 __allegro_KEY_LWIN,       __allegro_KEY_RWIN,       __allegro_KEY_MENU
};


/* convert Allegro format scancodes into key_shifts flag bits */
static unsigned short modifier_table[__allegro_KEY_MAX - __allegro_KEY_MODIFIERS] =
{
   __allegro_KB_SHIFT_FLAG,    __allegro_KB_SHIFT_FLAG,    __allegro_KB_CTRL_FLAG,
   __allegro_KB_CTRL_FLAG,     __allegro_KB_ALT_FLAG,      __allegro_KB_ALT_FLAG,
   __allegro_KB_LWIN_FLAG,     __allegro_KB_RWIN_FLAG,     __allegro_KB_MENU_FLAG,
   __allegro_KB_SCROLOCK_FLAG, __allegro_KB_NUMLOCK_FLAG,  __allegro_KB_CAPSLOCK_FLAG
};

#define NUM_PAD_KEYS 17
static char pad_asciis_numlock[NUM_PAD_KEYS] = "0123456789+-*/\r,.";
static char pad_asciis_no_numlock[NUM_PAD_KEYS] = { 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	'+', '-', '*', '/', '\r', -1, -1
};


static void process_keyboard_data (unsigned char *buf, size_t bytes_read) 
{
        unsigned int ch;
        unsigned int code, mycode, press;
        int ascii;
        int map;
	int val;
        struct kbentry kbe;

        ASSERT(buf || bytes_read==0);

        /* Process all keys read from left to right */
        for (ch = 0; ch < bytes_read; ch++) {
                /* Read kernel's fake scancode */
                code = buf[ch] & 0x7f;
                press = !(buf[ch] & 0x80);

                /* Convert to Allegro's fake scancode */
                mycode = kernel_to_mycode[code];

                /* Process modifiers */
                if (mycode >= __allegro_KEY_MODIFIERS) {
                        int flag = modifier_table[mycode - __allegro_KEY_MODIFIERS];
                        if (press) {
                                if (flag & KB_MODIFIERS)
                                        _key_shifts |= flag;
                                else if ((flag & KB_LED_FLAGS) && key_led_flag)
                                        _key_shifts ^= flag;
                                _handle_key_press (-1, mycode);
                        } else {
                                if (flag & KB_MODIFIERS)
                                        _key_shifts &= ~flag;
                                _handle_key_release (mycode);
                        }
                        continue;
                }

                /* Releases only need the scancode */
                if (!press) {
                        _handle_key_release (mycode);
                        continue;
                }

                /* Build kernel keymap number */
                map = 0;
                if (key[__allegro_KEY_LSHIFT] || key[__allegro_KEY_RSHIFT]) map |= 1;
                if (key[__allegro_KEY_ALTGR]) map |= 2;
                if (key[__allegro_KEY_LCONTROL] || key[__allegro_KEY_RCONTROL]) map |= 4;
                if (key[__allegro_KEY_ALT]) map |= 8;

                /* Map scancode to type and value */
                kbe.kb_table = map;
                kbe.kb_index = code;
                ioctl (std_keyboard.fd, KDGKBENT, &kbe);

		/* debugging */
		/* TRACE(PREFIX_I "keypress: code=%3d mycode=%3d map=%2d kb_value=%04x\n",
		      code, mycode, map, kbe.kb_value); */

                /* Allegro wants Alt+key to return ASCII code zero */
                if (key[__allegro_KEY_ALT])
                        ascii = 0;
		else
		/* Otherwise fill it in according to the key pressed */
		     switch (mycode) {
			case __allegro_KEY_BACKSPACE: ascii = 8; break;
			default:
				/* Most keys come here */
				ascii = KVAL(kbe.kb_value);
                		/* Handle invalid keymaps */
		                /* FIXME: Maybe we should just redo the
				 *        ioctl with keymap 0? */
				if (kbe.kb_value == K_NOSUCHMAP)
					ascii = 0;
		}

                /* Finally do some things based on key type */
                switch (KTYP(kbe.kb_value)) {
                        case KT_CONS:
                                if (!ioctl (__al_linux_console_fd, VT_ACTIVATE, KVAL(kbe.kb_value) + 1)) {
                                        _handle_key_press (-1, mycode);
                                        continue;
                                }
                                ascii = 0;
                                break;
                        case KT_LETTER:
                                /* Apply capslock translation */
                                if (_key_shifts & __allegro_KB_CAPSLOCK_FLAG)
                                        ascii ^= 0x20;
                                break;
                        case KT_LATIN:
                        case KT_ASCII:
                                break;
                        case KT_PAD:
                                ascii = -1;
				val = KVAL(kbe.kb_value);
				if ((val >= 0) && (val < NUM_PAD_KEYS)) {
					if (_key_shifts & __allegro_KB_NUMLOCK_FLAG)
                                                ascii = pad_asciis_numlock[val];
					else
						ascii = pad_asciis_no_numlock[val];
				}
                                break;
                        case KT_SPEC:
                                if (mycode == __allegro_KEY_ENTER) {
                                        ascii = '\r';
                                        break;
                                }
                        case KT_FN:
                        case KT_CUR:
                                ascii = -1;
                                break;
                        default:
                                /* Don't put anything in the buffer */
                                _handle_key_press (-1, mycode);
                }

                /* Replace -1 with the modifier state */
                if (ascii == -1)
                        ascii = _key_shifts & KB_MODIFIERS;

                /* three-finger salute for killing the program */
                if (((mycode == __allegro_KEY_DEL) || (mycode == __allegro_KEY_END)) && 
                    (three_finger_flag) &&
                    (_key_shifts & __allegro_KB_CTRL_FLAG) && 
                    (_key_shifts & __allegro_KB_ALT_FLAG)) {
                        TRACE(PREFIX_W "Three finger combo detected. SIGTERMing "
                              "pid %d\n", main_pid);
                        kill(main_pid, SIGTERM);
                }

                _handle_key_press (ascii, mycode);
        }
}

/* update_keyboard:
 *  Read keys from keyboard, if there are any, and update all relevant
 *  variables.
 *
 *  We take full advantage of the kernel keyboard driver while still working
 *  in the MEDIUM_RAW mode.  This allows us to both keep track of the up/down
 *  status of any key and return character codes from the kernel keymaps.
 */
static int update_keyboard (void)
{
   unsigned char buf[128];
   int bytes_read;
   fd_set set;
   struct timeval tv = { 0, 0 };

   if (resume_count <= 0)
      return 0;

   FD_ZERO(&set);
   FD_SET(std_keyboard.fd, &set);
   if (select (FD_SETSIZE, &set, NULL, NULL, &tv) <= 0)
      return 0;

   bytes_read = read(std_keyboard.fd, &buf, sizeof(buf));
   if (bytes_read < 1)
      return 0;

   process_keyboard_data(buf, bytes_read);

   return 1;
}


/* activate_keyboard:
 *  Put keyboard into the mode we want.
 */
static void activate_keyboard (void)
{
        tcsetattr (std_keyboard.fd, TCSANOW, &__al_linux_work_termio);
        ioctl (std_keyboard.fd, KDSKBMODE, K_MEDIUMRAW);
}

/* deactivate_keyboard:
 *  Restore the original keyboard settings.
 */
static void deactivate_keyboard (void)
{
        tcsetattr (std_keyboard.fd, TCSANOW, &__al_linux_startup_termio);
        ioctl (std_keyboard.fd, KDSKBMODE, startup_kbmode);
}


/* release_all_keys:
 *  Sends release messages for all keys that are marked as pressed in the
 *  key array.  Maybe this should be a global function in `src/keyboard.c';
 *  it could be used by other platforms, and could work better with the
 *  polled driver emulation system.
 */
static void release_all_keys (void)
{
        int i;
        for (i = 0; i < __allegro_KEY_MAX; i++)
                if (key[i]) _handle_key_release (i);
}

/* suspend_keyboard:
 *  Suspend our keyboard handling, restoring the original OS values, so that
 *  the application can take advantage of Linux keyboard handling.
 */
static void suspend_keyboard (void)
{
        resume_count--;
        if (resume_count == 0)
                deactivate_keyboard();
        release_all_keys();
}

/* resume_keyboard:
 *  Start/resume Allegro keyboard handling.
 */
static void resume_keyboard(void)
{
        if (resume_count == 0)
                activate_keyboard();
        resume_count++;
}


static int linux_key_init (void)
{
        if (__al_linux_use_console()) return 1;

        std_keyboard.fd = dup (__al_linux_console_fd);

        /* Set terminal attributes we need */
        __al_linux_work_termio.c_iflag = 0;
        __al_linux_work_termio.c_cflag = CS8;
        __al_linux_work_termio.c_lflag = 0;

        /* Prevent `read' from blocking */
        __al_linux_work_termio.c_cc[VMIN] = 0;
        __al_linux_work_termio.c_cc[VTIME] = 0;

        /* Save previous keyboard mode (probably XLATE) */
        ioctl (std_keyboard.fd, KDGKBMODE, &startup_kbmode);

        /* Keyboard is disabled */
        resume_count = 0;

        /* Get the pid, which we use for the three finger salute */
        main_pid = getpid();

        /* Register our driver (enables it too) */
        __al_linux_add_standard_driver (&std_keyboard);

        return 0;
}

static void linux_key_exit (void)
{
	/* Reset LED mode before exiting */
	ioctl(std_keyboard.fd, KDSETLED, 8);

        __al_linux_remove_standard_driver (&std_keyboard);
        close (std_keyboard.fd);

        __al_linux_leave_console();
}

static void linux_set_leds (int leds)
{
	int val = 0;
	if (leds & __allegro_KB_SCROLOCK_FLAG) val |= LED_SCR;
	if (leds & __allegro_KB_NUMLOCK_FLAG) val |= LED_NUM;
	if (leds & __allegro_KB_CAPSLOCK_FLAG) val |= LED_CAP;
        ioctl(std_keyboard.fd, KDSETLED, val);
}

static void linux_set_rate (int delay, int rate)
{
}

static int linux_scancode_to_ascii (int scancode)
{
        int kernel_code;
        struct kbentry kbe;

        for (kernel_code = 0; kernel_code < 128; kernel_code++)
                if (scancode == kernel_to_mycode[kernel_code]) break;
        if (kernel_code == 128) return 0;

        kbe.kb_table = 0;
        kbe.kb_index = kernel_code;
        ioctl (std_keyboard.fd, KDGKBENT, &kbe);
        
        switch (KTYP(kbe.kb_value)) {
                case KT_LETTER:
                case KT_LATIN:
                case KT_ASCII:
                        return KVAL(kbe.kb_value);
                case KT_SPEC:
                        if (scancode == __allegro_KEY_ENTER)
                                return '\r';
                        break;
        }
        return 0;
}

