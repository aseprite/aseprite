// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_ALERT_H_INCLUDED
#define UI_ALERT_H_INCLUDED
#pragma once

#include "base/shared_ptr.h"
#include "ui/window.h"

namespace ui {

  class Box;
  class Slider;

  class Alert;
  typedef base::SharedPtr<Alert> AlertPtr;

  class Alert : public Window {
  public:
    Alert();

    void addProgress();
    void setProgress(double progress);

    static AlertPtr create(const char* format, ...);
    static int show(const char* format, ...);

  private:
    void processString(char* buf, std::vector<Widget*>& labels, std::vector<Widget*>& buttons);

    Slider* m_progress;
    Box* m_progressPlaceholder;
  };

} // namespace ui

#endif
