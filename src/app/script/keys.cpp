// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/keys.h"

namespace app {
namespace script {

// Same order that os::KeyScancode
// Based on code values of the KeyboardEvent on web code:
// https://developer.mozilla.org/en-US/docs/Web/API/UI_Events/Keyboard_event_code_values
static const char* vkcode_to_code_table[] = {
  "Unidentified",
  "KeyA",
  "KeyB",
  "KeyC",
  "KeyD",
  "KeyE",
  "KeyF",
  "KeyG",
  "KeyH",
  "KeyI",
  "KeyJ",
  "KeyK",
  "KeyL",
  "KeyM",
  "KeyN",
  "KeyO",
  "KeyP",
  "KeyQ",
  "KeyR",
  "KeyS",
  "KeyT",
  "KeyU",
  "KeyV",
  "KeyW",
  "KeyX",
  "KeyY",
  "KeyZ",
  "Digit0",
  "Digit1",
  "Digit2",
  "Digit3",
  "Digit4",
  "Digit5",
  "Digit6",
  "Digit7",
  "Digit8",
  "Digit9",
  "Numpad0",
  "Numpad1",
  "Numpad2",
  "Numpad3",
  "Numpad4",
  "Numpad5",
  "Numpad6",
  "Numpad7",
  "Numpad8",
  "Numpad9",
  "F1",
  "F2",
  "F3",
  "F4",
  "F5",
  "F6",
  "F7",
  "F8",
  "F9",
  "F10",
  "F11",
  "F12",
  "Escape",
  "Backquote",
  "Minus",
  "Equal",
  "Backspace",
  "Tab",
  "BracketLeft",
  "BracketRight",
  "Enter",
  "Semicolon",
  "Quote",
  "Backslash",
  nullptr, // kKeyBackslash2,
  "Comma",
  "Period",
  "Slash",
  "Space",
  "Insert",
  "Delete",
  "Home",
  "End",
  "PageUp",
  "PageDown",
  "ArrowLeft",
  "ArrowRight",
  "ArrowUp",
  "ArrowDown",
  "NumpadDivide",
  "NumpadMultiply",
  "NumpadSubtract",
  "NumpadAdd",
  "NumpadComma",
  "NumpadEnter",
  "PrintScreen",
  "Pause",
  nullptr, // kKeyAbntC1
  "IntlYen",
  "KanaMode",
  "Convert",
  "NonConvert",
  nullptr, // kKeyAt
  nullptr, // kKeyCircumflex
  nullptr, // kKeyColon2
  nullptr, // kKeyKanji
  "NumpadEqual", // kKeyEqualsPad
  "Backquote",
  nullptr, // kKeySemicolon
  nullptr, // kKeyUnknown1
  nullptr, // kKeyUnknown2
  nullptr, // kKeyUnknown3
  nullptr, // kKeyUnknown4
  nullptr, // kKeyUnknown5
  nullptr, // kKeyUnknown6
  nullptr, // kKeyUnknown7
  nullptr, // kKeyUnknown8
  "ShiftLeft",
  "ShiftRight",
  "ControlLeft",
  "ControlRight",
  "AltLeft",
  "AltRight"
  "MetaLeft",
  "MetaRight",
  "ContextMenu",
  "MetaLeft", // kKeyCommand
  "ScrollLock",
  "NumLock",
  "CapsLock",
};

static int vkcode_to_code_table_size =
  sizeof(vkcode_to_code_table) / sizeof(vkcode_to_code_table[0]);

const char* vkcode_to_code(const ui::KeyScancode vkcode)
{
  if (vkcode >= 0 &&
      vkcode < vkcode_to_code_table_size &&
      vkcode_to_code_table[vkcode]) {
    return vkcode_to_code_table[vkcode];
  }
  else {
    return vkcode_to_code_table[0];
  }
}

} // namespace script
} // namespace app
