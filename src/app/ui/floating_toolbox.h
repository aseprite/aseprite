// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_FLOATING_TOOLBOX_H_INCLUDED
#define APP_UI_FLOATING_TOOLBOX_H_INCLUDED
#pragma once

#include "app/tools/active_tool_observer.h"
#include "app/ui/floating_window.h"

#include <vector>

namespace app {

namespace tools {
class Tool;
}

class FloatingToolbox : public FloatingWindow,
                        public tools::ActiveToolObserver {
public:
  FloatingToolbox();
  ~FloatingToolbox();

protected:
  void onPaint(ui::PaintEvent& ev) override;
  bool onProcessMessage(ui::Message* msg) override;
  void onSizeHint(ui::SizeHintEvent& ev) override;
  void onResize(ui::ResizeEvent& ev) override;

  // ActiveToolObserver
  void onActiveToolChange(tools::Tool* tool) override;
  void onSelectedToolChange(tools::Tool* tool) override;

private:
  gfx::Size getToolIconSize() const;
  int getColumns(int width) const;
  gfx::Rect toolBounds(int index) const;
  tools::Tool* hitTestTool(const gfx::Point& pt) const;
  void fillToolList();

  std::vector<tools::Tool*> m_tools;
  tools::Tool* m_hotTool;
};

} // namespace app

#endif // APP_UI_FLOATING_TOOLBOX_H_INCLUDED
