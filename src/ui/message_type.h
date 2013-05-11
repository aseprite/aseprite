// ASEPRITE gui library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UI_MESSAGE_TYPE_H_INCLUDED
#define UI_MESSAGE_TYPE_H_INCLUDED

namespace ui {

  // Message types.
  enum MessageType {
    // General messages.
    kOpenMessage,     // Windows is open.
    kCloseMessage,    // Windows is closed.
    kCloseAppMessage, // The user wants to close the entire application.
    kPaintMessage,    // Widget needs be repainted.
    kTimerMessage,    // A timer timeout.
    kWinMoveMessage,  // Window movement.
    kQueueProcessingMessage,    // Only sent to manager which indicate
                                // the last message in the queue.

    // Keyboard related messages.
    kKeyDownMessage,         // When a any key is pressed.
    kKeyUpMessage,           // When a any key is released.
    kFocusEnterMessage,      // Widget gets the focus.
    kFocusLeaveMessage,      // Widget losts the focus.

    // Mouse related messages.
    kMouseDownMessage,      // User makes click inside a widget.
    kMouseUpMessage,        // User releases mouse button in a widget.
    kDoubleClickMessage,    // User makes double click in some widget.
    kMouseEnterMessage,     // A widget gets mouse pointer.
    kMouseLeaveMessage,     // A widget losts mouse pointer.
    kMouseMoveMessage,      // User moves the mouse on some widget.
    kSetCursorMessage,      // A widget needs to setup the mouse cursor.
    kMouseWheelMessage,     // User moves the wheel.

    // TODO Drag'n'drop messages...
    // k...DndMessage

    // User widgets.
    kFirstRegisteredMessage,
    kLastRegisteredMessage = 0x7fffffff
  };

} // namespace ui

#endif  // UI_MESSAGE_TYPE_H_INCLUDED
