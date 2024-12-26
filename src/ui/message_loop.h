// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_MESSAGE_LOOP_H_INCLUDED
#define UI_MESSAGE_LOOP_H_INCLUDED
#pragma once

namespace ui {

class Manager;

class MessageLoop {
public:
  MessageLoop(Manager* manager);

  void pumpMessages();

private:
  Manager* m_manager;
};

} // namespace ui

#endif
