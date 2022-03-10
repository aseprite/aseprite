// Aseprite
// Copyright (C) 2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/dragging_value_state.h"

#include "app/ui/editor/editor.h"
#include "base/clamp.h"
#include "ui/display.h"
#include "ui/message.h"
#include "ui/scale.h"
#include "ui/system.h"

#include <cmath>

namespace app {

using namespace ui;

DraggingValueState::DraggingValueState(Editor* editor, const Keys& keys)
  : m_keys(keys)
  , m_initialPos(editor->display()->nativeWindow()->pointFromScreen(ui::get_mouse_position()))
  , m_initialFgColor(StateWithWheelBehavior::initialFgColor())
  , m_initialBgColor(StateWithWheelBehavior::initialBgColor())
  , m_initialFgTileIndex(StateWithWheelBehavior::initialFgTileIndex())
  , m_initialBgTileIndex(StateWithWheelBehavior::initialBgTileIndex())
  , m_initialBrushSize(StateWithWheelBehavior::initialBrushSize())
  , m_initialBrushAngle(StateWithWheelBehavior::initialBrushAngle())
  , m_initialScroll(StateWithWheelBehavior::initialScroll(editor))
  , m_initialZoom(StateWithWheelBehavior::initialZoom(editor))
  , m_initialFrame(StateWithWheelBehavior::initialFrame(editor))
  , m_initialInkType(StateWithWheelBehavior::initialInkType(editor))
  , m_initialInkOpacity(StateWithWheelBehavior::initialInkOpacity(editor))
  , m_initialCelOpacity(StateWithWheelBehavior::initialCelOpacity(editor))
  , m_initialLayerOpacity(StateWithWheelBehavior::initialLayerOpacity(editor))
  , m_initialTool(StateWithWheelBehavior::initialTool())
{
  if (!editor->hasCapture())
    editor->captureMouse();

  // As StateWithWheelBehavior::initialLayer() fills browsableLayers()
  // we will only fill it if it's necessary (there is a key that
  // triggers WheelAction::Layer)
  for (const KeyPtr& key : m_keys) {
    if (key->wheelAction() == WheelAction::Layer) {
      m_initialLayer = StateWithWheelBehavior::initialLayer(editor);
      break;
    }
  }
}

bool DraggingValueState::onMouseDown(Editor* editor, MouseMessage* msg)
{
  return true;
}

bool DraggingValueState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  editor->backToPreviousState();
  editor->releaseMouse();
  return true;
}

bool DraggingValueState::onMouseMove(Editor* editor, MouseMessage* msg)
{
  m_fgColor = m_initialFgColor;

  for (const KeyPtr& key : m_keys) {
    const gfx::Point delta = (msg->position() - m_initialPos);
    const DragVector deltaV(delta.x, delta.y);
    const DragVector invDragVector(key->dragVector().x,
                                   -key->dragVector().y);
    const double threshold = invDragVector.magnitude();

    DragVector v = deltaV.projectOn(invDragVector);
    double dz = v.magnitude();
    {
      if (threshold > 0)
        dz /= threshold;
      auto dot = invDragVector * v;
      dz *= SGN(dot);

      bool preciseWheel = true;
      if (key->wheelAction() == WheelAction::Zoom ||
          key->wheelAction() == WheelAction::Frame ||
          key->wheelAction() == WheelAction::Layer) {
        preciseWheel = false;
        dz = -dz; // Invert value for zoom only so the vector is
                  // pointing to the direction to increase zoom

        // TODO we should change the direction of the wheel
        //      information from the laf layer
      }
      else if (key->wheelAction() == WheelAction::InkType) {
        preciseWheel = false;
      }

      processWheelAction(editor,
                         key->wheelAction(),
                         msg->position(),
                         delta,
                         dz,
                         false, // scrollBigSteps=false
                         preciseWheel);
    }
  }

  if (m_fgColor != m_initialFgColor)
    StateWithWheelBehavior::changeFgColor(m_fgColor);

  return true;
}

bool DraggingValueState::onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos)
{
  return StateWithWheelBehavior::onSetCursor(editor, mouseScreenPos);
}

bool DraggingValueState::onKeyDown(Editor* editor, KeyMessage* msg)
{
  return false;
}

bool DraggingValueState::onKeyUp(Editor* editor, KeyMessage* msg)
{
  if (editor->hasCapture())
    editor->releaseMouse();
  editor->backToPreviousState();
  return true;
}

bool DraggingValueState::onUpdateStatusBar(Editor* editor)
{
  return false;
}

void DraggingValueState::changeFgColor(Color c)
{
  m_fgColor = c;
}

} // namespace app
