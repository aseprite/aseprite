// Aseprite UI Library
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_ALERT_H_INCLUDED
#define UI_ALERT_H_INCLUDED
#pragma once

#include "base/shared_ptr.h"
#include "ui/window.h"

#include <string>
#include <vector>

namespace ui {

  class Box;
  class CheckBox;
  class Slider;

  class Alert;
  typedef base::SharedPtr<Alert> AlertPtr;

  class Alert : public Window {
  public:
    Alert();

    void setTitle(const std::string& title);
    void addLabel(const std::string& text, const int align);
    void addSeparator();
    void addButton(const std::string& text);

    void addProgress();
    void setProgress(double progress);

    CheckBox* addCheckBox(const std::string& text);

    int show();

    static AlertPtr create(const std::string& msg);
    static int show(const std::string& msg);

  private:
    void processString(std::string& buf);

    Slider* m_progress;
    Box* m_labelsPlaceholder;
    Box* m_buttonsPlaceholder;
    Box* m_progressPlaceholder;
    std::vector<Widget*> m_buttons;
  };

} // namespace ui

#endif
