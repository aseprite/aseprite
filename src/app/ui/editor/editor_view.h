// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_VIEW_H_INCLUDED
#define APP_UI_EDITOR_VIEW_H_INCLUDED
#pragma once

#include "obs/connection.h"
#include "ui/view.h"

namespace app {
class Editor;

class EditorView : public ui::View {
public:
  enum Type { CurrentEditorMode, AlwaysSelected };

  enum Method { KeepOrigin, KeepCenter };
  static void SetScrollUpdateMethod(Method method);

  EditorView(Type type);

protected:
  void onPaint(ui::PaintEvent& ev) override;
  void onResize(ui::ResizeEvent& ev) override;
  void onSetViewScroll(const gfx::Point& pt) override;
  void onScrollRegion(ui::ScrollRegionEvent& ev) override;
  void onScrollChange() override;

private:
  Editor* editor();
  void setupScrollbars();

  Type m_type;
  obs::scoped_connection m_scrollSettingsConn;
  static Method g_scrollUpdateMethod;
};

} // namespace app

#endif // APP_UI_EDITOR_VIEW_H_INCLUDED
