// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.


#include "config.h"

#include "gui/jcustom_label.h"

CustomLabel::CustomLabel(const char *text)
  : Label(text)
{
}

bool CustomLabel::onProcessMessage(JMessage msg)
{
  return Label::onProcessMessage(msg);
}
