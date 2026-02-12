// Aseprite UI Library
// Copyright (C) 2019-2025  Igara Studio S.A.
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
  kPaintMessage,        // Widget needs be repainted.
  kTimerMessage,        // A timer timeout.
  kDropFilesMessage,    // Drop files in the manager.
  kWinMoveMessage,      // Window movement.
  kAppLostFocusMessage, // App lost focus.

  // Keyboard related messages.
  kKeyDownMessage,    // When a any key is pressed.
  kKeyUpMessage,      // When a any key is released.
  kFocusEnterMessage, // Widget gets the focus.
  kFocusLeaveMessage, // Widget losts the focus.

  // Mouse related messages.
  kMouseDownMessage,   // User makes click inside a widget.
  kMouseUpMessage,     // User releases mouse button in a widget.
  kDoubleClickMessage, // User makes double click in some widget.
  kMouseEnterMessage,  // A widget gets mouse pointer.
  kMouseLeaveMessage,  // A widget losts mouse pointer.
  kMouseMoveMessage,   // User moves the mouse on some widget.
  kSetCursorMessage,   // A widget needs to setup the mouse cursor.
  kMouseWheelMessage,  // User moves the wheel.

  // Touch related messages.
  kTouchMagnifyMessage,

  // Drag'n'drop messages.
  kDragEnterMessage, // Mouse pointer entered a drop-target widget's bounds while the user is
                     // dragging elements.
  kDragLeaveMessage, // Mouse pointer left a drop-target widget's bounds while the user is
                     // dragging elements.
  kDragMessage, // Mouse pointer is being moved inside of a drop-target widget's bounds while the
                // user is dragging elements.
  kDropMessage, // User dropped elements inside a drop-target widget's bounds.

  // Call a generic function when we are processing the queue of
  // messages. Used to process an laf-os event in the same order
  // they were received in some cases (e.g. mouse leave/enter).
  kCallbackMessage,

  // User widgets.
  kFirstRegisteredMessage,
  kLastRegisteredMessage = 0x7fffffff
};

} // namespace ui

#endif // UI_MESSAGE_TYPE_H_INCLUDED
