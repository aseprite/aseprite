// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef UI_REGISTER_MESSAGE_H_INCLUDED
#define UI_REGISTER_MESSAGE_H_INCLUDED
#pragma once

#include "ui/message_type.h"

namespace ui {

  class RegisterMessage
  {
  public:
    RegisterMessage();
    operator MessageType() { return m_type; }
  private:
    MessageType m_type;
  };

} // namespace ui

#endif
