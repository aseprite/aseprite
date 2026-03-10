// Aseprite UI Library
// Copyright (C) 2019-present  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_MESSAGE_TYPE_H_INCLUDED
#define UI_MESSAGE_TYPE_H_INCLUDED
#pragma once

namespace ui {

// Message types.
enum MessageType {
  // General messages.
  kOpenMessage,         // Windows is open.
  kCloseMessage,        // Windows is closed.
  kCloseDisplayMessage, // The user wants to close the entire application.
  kResizeDisplayMessage,
  kPaintMessage,     // A widget needs to be repainted.
  kTimerMessage,     // A timer timeout.
  kDropFilesMessage, // Drop files in the manager.
  kWinMoveMessage,   // Window movement.

  // Keyboard related messages.
  kKeyDownMessage,    // When a key is pressed.
  kKeyUpMessage,      // When a key is released.
  kFocusEnterMessage, // A widget gets the focus.
  kFocusLeaveMessage, // A widget loses the focus.

  // Mouse related messages.
  kMouseDownMessage,   // The user clicks inside a widget.
  kMouseUpMessage,     // The user releases a mouse button from a widget.
  kDoubleClickMessage, // The user double clicks in some widget.
  kMouseEnterMessage,  // A widget gets the mouse pointer.
  kMouseLeaveMessage,  // A widget loses the mouse pointer.
  kMouseMoveMessage,   // The user moves the mouse on some widget.
  kSetCursorMessage,   // A widget needs to specify its mouse cursor.
  kMouseWheelMessage,  // The user moves the wheel.

  // Touch related messages.
  kTouchMagnifyMessage,

  // Drag'n'drop messages.
  kDragEnterMessage, // Mouse pointer entered a drop-target widget's bounds while the user is
                     // dragging elements.
  kDragLeaveMessage, // Mouse pointer left a drop-target widget's bounds while the user is
                     // dragging elements.
  kDragMessage, // Mouse pointer is being moved inside of a drop-target widget's bounds while the
                // user is dragging elements.
  kDropMessage, // The user dropped elements inside a drop-target widget's bounds.

  // Calls a generic function when we are processing the queue of
  // messages. Used to process an laf-os event in the same order
  // they were received in some cases (e.g. mouse leave/enter).
  kCallbackMessage,

  // User-defined messages with ui::RegisterMessage.
  kFirstRegisteredMessage,
  kLastRegisteredMessage = 0x7fffffff
};

} // namespace ui

#endif // UI_MESSAGE_TYPE_H_INCLUDED
