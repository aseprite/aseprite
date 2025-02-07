// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/window_with_hand.h"

#include "app/app.h"
#include "app/tools/active_tool.h"
#include "app/tools/tool_box.h"
#include "app/ui/context_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui/toolbar.h"

namespace app {

WindowWithHand::WindowWithHand(Type type, const std::string& text) : Window(type, text)
{
}

WindowWithHand::~WindowWithHand()
{
  enableHandTool(false);
}

void WindowWithHand::enableHandTool(const bool state)
{
  auto* atm = App::instance()->activeToolManager();

  if (m_editor) {
    m_editor->remove_observer(this);
    m_editor = nullptr;
  }
  if (m_oldTool) {
    atm->setAllowQuickToolChanges(true);
    ToolBar::instance()->selectTool(m_oldTool);
    m_oldTool = nullptr;
  }

  auto* editor = Editor::activeEditor();
  if (state && editor) {
    m_editor = editor;
    m_editor->add_observer(this);

    // Disable quick tools like Ctrl to select the Move tool and move
    // cels, or Alt for eyedropper. We just want the Hand tool
    // (selected tool) for preview purposes.
    atm->setAllowQuickToolChanges(false);
    m_oldTool = m_editor->getCurrentEditorTool();

    tools::Tool* hand = App::instance()->toolBox()->getToolById(tools::WellKnownTools::Hand);
    ToolBar::instance()->selectTool(hand);
  }
}

void WindowWithHand::onBroadcastMouseMessage(const gfx::Point& screenPos, ui::WidgetsList& targets)
{
  if (m_editor) {
    // Add this window as receptor of mouse events.
    targets.push_back(this);
    // Also add the editor
    if (m_editor)
      targets.push_back(ui::View::getView(m_editor));
    // and add the context bar.
    if (App::instance()->contextBar())
      targets.push_back(App::instance()->contextBar());
  }
  else {
    Window::onBroadcastMouseMessage(screenPos, targets);
  }
}

} // namespace app
