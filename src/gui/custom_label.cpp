// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.


#include "config.h"

#include "gui/custom_label.h"

CustomLabel::CustomLabel(const char *text)
  : Label(text)
{
}

bool CustomLabel::onProcessMessage(Message* msg)
{
  return Label::onProcessMessage(msg);
}
