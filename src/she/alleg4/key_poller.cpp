// SHE library
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/she.h"

#include "she/alleg4/scancode.h"

#include <allegro.h>

namespace she {

namespace {

int key_repeated[kKeyScancodes];

int she_keyboard_ucallback(int unicode_char, int* scancode)
{
  KeyScancode she_scancode =
    alleg_to_she_scancode((*scancode) & 0x7f);

  Event ev;
  ev.setType(Event::KeyDown);
  ev.setScancode(she_scancode);
  if (unicode_char > 0)
    ev.setUnicodeChar(unicode_char);
  ev.setRepeat(key_repeated[she_scancode]++);
  queue_event(ev);

  return unicode_char;
}

void she_keyboard_lowlevel_callback(int scancode)
{
  // Bit 0x80 indicates that it is a key release.
#ifdef ALLEGRO_UNIX
  if (!(scancode & 0x80)) {
    // Generate KeyDown events for modifiers. Needed for Allegro 4 on
    // Linux as modifiers don't generate keyboard_ucallback() calls.
    if (scancode == KEY_CAPSLOCK || scancode == KEY_COMMAND ||
        scancode == KEY_LSHIFT   || scancode == KEY_RSHIFT ||
        scancode == KEY_LCONTROL || scancode == KEY_RCONTROL ||
        scancode == KEY_ALT      || scancode == KEY_ALTGR ||
        scancode == KEY_LWIN     || scancode == KEY_RWIN) {
      she_keyboard_ucallback(-1, &scancode);
    }
    return;
  }
#endif

  scancode ^= 0x80;
  KeyScancode she_scancode = alleg_to_she_scancode(scancode);
  key_repeated[she_scancode] = 0;

  Event ev;
  ev.setType(Event::KeyUp);
  ev.setScancode(she_scancode);
  ev.setUnicodeChar(scancode_to_ascii(scancode));
  ev.setRepeat(0);
  queue_event(ev);
}

}

void key_poller_init()
{
  for (int c=0; c<kKeyScancodes; c++)
    key_repeated[c] = 0;

  keyboard_ucallback = she_keyboard_ucallback;
  keyboard_lowlevel_callback = she_keyboard_lowlevel_callback;
}

void key_poller_generate_events()
{
  // Just ignore the whole keyboard buffer (we use callbacks).
  clear_keybuf();
}

} // namespace she
