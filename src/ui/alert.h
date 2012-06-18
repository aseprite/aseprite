// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UI_ALERT_H_INCLUDED
#define UI_ALERT_H_INCLUDED

#include "base/shared_ptr.h"
#include "ui/frame.h"

namespace ui {

  class Alert;
  typedef SharedPtr<Alert> AlertPtr;

  class Alert : public Frame
  {
  public:
    Alert();

    static AlertPtr create(const char* format, ...);
    static int show(const char* format, ...);

  private:
    void processString(char* buf, std::vector<Widget*>& labels, std::vector<Widget*>& buttons);
  };

} // namespace ui

#endif
