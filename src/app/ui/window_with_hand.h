// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_WINDOW_WITH_HAND_H_INCLUDED
#define APP_UI_WINDOW_WITH_HAND_H_INCLUDED
#pragma once

#include "app/ui/editor/editor_observer.h"
#include "ui/window.h"

namespace app {
namespace tools {
class Tool;
}

// A Window that enables the possibility to scroll/zoom the active
// editor with the Hand tool.
class WindowWithHand : public ui::Window,
                       public EditorObserver {
public:
  WindowWithHand(Type type, const std::string& text);
  ~WindowWithHand();

  // Enables the Hand tool in the active editor.
  void enableHandTool(bool state);

  bool isHandToolEnabled() const { return m_editor != nullptr; }

protected:
  void onBroadcastMouseMessage(const gfx::Point& screenPos, ui::WidgetsList& targets) override;

private:
  Editor* m_editor = nullptr;
  tools::Tool* m_oldTool = nullptr;
};

} // namespace app

#endif
