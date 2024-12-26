// Aseprite
// Copyright (C) 2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_DRAGGING_VALUE_STATE_H_INCLUDED
#define APP_UI_EDITOR_DRAGGING_VALUE_STATE_H_INCLUDED
#pragma once

#include "app/ui/editor/state_with_wheel_behavior.h"
#include "app/ui/key.h"
#include "gfx/point.h"
#include "obs/connection.h"
#include "render/zoom.h"

namespace app {
class CommandExecutionEvent;

class DraggingValueState : public StateWithWheelBehavior {
public:
  DraggingValueState(Editor* editor, const Keys& keys);

  bool isTemporalState() const override { return true; }
  void onBeforePopState(Editor* editor) override;
  bool onMouseDown(Editor* editor, ui::MouseMessage* msg) override;
  bool onMouseUp(Editor* editor, ui::MouseMessage* msg) override;
  bool onMouseMove(Editor* editor, ui::MouseMessage* msg) override;
  bool onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos) override;
  bool onKeyDown(Editor* editor, ui::KeyMessage* msg) override;
  bool onKeyUp(Editor* editor, ui::KeyMessage* msg) override;
  bool onUpdateStatusBar(Editor* editor) override;

  bool requireBrushPreview() override { return true; }

private:
  void onBeforeCommandExecution(CommandExecutionEvent& ev);
  Color initialFgColor() const override { return m_fgColor; }
  Color initialBgColor() const override { return m_initialBgColor; }
  int initialFgTileIndex() const override { return m_initialFgTileIndex; }
  int initialBgTileIndex() const override { return m_initialBgTileIndex; }
  int initialBrushSize() override { return m_initialBrushSize; }
  int initialBrushAngle() override { return m_initialBrushAngle; }
  gfx::Point initialScroll(Editor* editor) const override { return m_initialScroll; }
  render::Zoom initialZoom(Editor* editor) const override { return m_initialZoom; }
  doc::frame_t initialFrame(Editor* editor) const override { return m_initialFrame; }
  doc::layer_t initialLayer(Editor* editor) const override { return m_initialLayer; }
  tools::InkType initialInkType(Editor* editor) const override { return m_initialInkType; }
  int initialInkOpacity(Editor* editor) const override { return m_initialInkOpacity; }
  int initialCelOpacity(Editor* editor) const override { return m_initialCelOpacity; }
  int initialLayerOpacity(Editor* editor) const override { return m_initialLayerOpacity; }
  tools::Tool* initialTool() const override { return m_initialTool; }
  void changeFgColor(Color c) override;

  tools::Tool* getInitialToolInActiveGroup() override;
  void onToolChange(tools::Tool* tool) override;
  void onToolGroupChange(Editor* editor, tools::ToolGroup* group) override;

  Editor* m_editor;
  Keys m_keys;
  gfx::Point m_initialPos;
  gfx::Point m_initialPosSameGroup;

  Color m_initialFgColor;
  Color m_initialBgColor;
  int m_initialFgTileIndex;
  int m_initialBgTileIndex;
  int m_initialBrushSize;
  int m_initialBrushAngle;
  gfx::Point m_initialScroll;
  render::Zoom m_initialZoom;
  doc::frame_t m_initialFrame;
  doc::layer_t m_initialLayer;
  tools::InkType m_initialInkType;
  int m_initialInkOpacity;
  int m_initialCelOpacity;
  int m_initialLayerOpacity;
  tools::Tool* m_initialTool;

  // Used to allow multiple modifications to the same initial FG
  // color (m_initialFgColor), e.g. when multiples Key will change
  // different elements of the color (e.g. Value and Saturation) at
  // the same time with different DragVectors/axes.
  Color m_fgColor;

  obs::scoped_connection m_beforeCmdConn;
};

} // namespace app

#endif // APP_UI_EDITOR_ZOOMING_STATE_H_INCLUDED
