// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UI_CUSTOM_LABEL_H_INCLUDED
#define UI_CUSTOM_LABEL_H_INCLUDED

#include "base/compiler_specific.h"
#include "ui/label.h"

namespace ui {

  class CustomLabel : public Label
  {
  public:
    CustomLabel(const char *text);

  protected:
    bool onProcessMessage(Message* msg) OVERRIDE;

  };

} // namespace ui

#endif
