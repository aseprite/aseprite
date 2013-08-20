// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef UI_MESSAGE_LOOP_H_INCLUDED
#define UI_MESSAGE_LOOP_H_INCLUDED

namespace ui {

  class Manager;

  class MessageLoop
  {
  public:
    MessageLoop(Manager* manager);

    void pumpMessages();

  private:
    Manager* m_manager;
  };

} // namespace ui

#endif
