// SHE library
// Copyright (C) 2012-2014  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/alleg4/internals.h"
#include "she/she.h"

#include <allegro.h>

namespace { 

static char old_readed_key[KEY_MAX]; // keyboard status of previous poll
static unsigned key_repeated[KEY_MAX];

}

namespace she {

void key_poller_init()
{
  // Reset keyboard
  for (int c=0; c<KEY_MAX; c++) {
    old_readed_key[c] = 0;
    key_repeated[c] = 0;
  }
}

void key_poller_generate_events()
{
  Event ev;

  // Poll keyboard
  poll_keyboard();

  // Generate kKeyDownMessage messages.
  int c;
  while (keypressed()) {
    int scancode;
    int unicode_char = ureadkey(&scancode);
    int repeat = 0;
    {
      c = scancode;
      if (c >= 0 && c < KEY_MAX) {
        old_readed_key[c] = key[c];
        repeat = key_repeated[c]++;
      }
    }

    ev.setType(Event::KeyDown);
    ev.setScancode(static_cast<KeyScancode>(scancode));
    if (unicode_char != 0)
      ev.setUnicodeChar(unicode_char);
    else
      ev.setUnicodeChar(::scancode_to_ascii(scancode));
    ev.setRepeat(repeat);
    queue_event(ev);
  }

  for (int c=0; c<KEY_MAX; c++) {
    if (old_readed_key[c] != key[c]) {
      KeyScancode scancode = static_cast<KeyScancode>(c);

      // Generate kKeyUpMessage messages (old key state is activated,
      // the new one is deactivated).
      if (old_readed_key[c]) {
        ev.setType(Event::KeyUp);
        ev.setScancode(scancode);
        ev.setUnicodeChar(::scancode_to_ascii(scancode));
        ev.setRepeat(0);
        queue_event(ev);

        old_readed_key[c] = key[c];
        key_repeated[c] = 0;
      }
      // Generate kKeyDownMessage messages for modifiers
      else if (c >= kKeyFirstModifierScancode || c == kKeyCommand) {
        ev.setType(Event::KeyDown);
        ev.setScancode(scancode);
        ev.setUnicodeChar(::scancode_to_ascii(scancode));
        ev.setRepeat(key_repeated[c]++);
        queue_event(ev);

        old_readed_key[c] = key[c];
      }
    }
  }
}

} // namespace she

