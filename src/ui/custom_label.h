// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_CUSTOM_LABEL_H_INCLUDED
#define UI_CUSTOM_LABEL_H_INCLUDED
#pragma once

#include "base/compiler_specific.h"
#include "ui/label.h"

namespace ui {

  class CustomLabel : public Label
  {
  public:
    CustomLabel(const base::string& text);

  protected:
    bool onProcessMessage(Message* msg) OVERRIDE;

  };

} // namespace ui

#endif
