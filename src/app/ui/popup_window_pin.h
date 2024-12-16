// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_POPUP_FRAME_PIN_H_INCLUDED
#define APP_UI_POPUP_FRAME_PIN_H_INCLUDED
#pragma once

#include "ui/popup_window.h"

namespace app {

class PopupWindowPin : public ui::PopupWindow {
public:
  PopupWindowPin(const std::string& text,
                 const ClickBehavior clickBehavior,
                 const bool canPin = false);

  bool isPinned() const { return m_pinned; }
  void setPinned(const bool pinned);

protected:
  virtual bool onProcessMessage(ui::Message* msg) override;
  virtual void onWindowMovement() override;

private:
  bool m_pinned;
};

} // namespace app

#endif
