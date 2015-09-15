// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/clear_rect.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/modules/editors.h"
#include "app/tools/ink.h"
#include "app/tools/tool_box.h"
#include "app/transaction.h"
#include "app/ui/context_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/select_box_state.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/main_window.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "app/util/new_image_from_mask.h"
#include "base/convert_to.h"
#include "doc/mask.h"

namespace app {

class NewBrushCommand : public Command
                      , public SelectBoxDelegate {
public:
  NewBrushCommand();
  Command* clone() const override { return new NewBrushCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;

  // SelectBoxDelegate impl
  void onQuickboxEnd(Editor* editor, const gfx::Rect& rect, ui::MouseButtons buttons) override;
  void onQuickboxCancel(Editor* editor) override;

  std::string onGetContextBarHelp() override {
    return "Select brush bounds | Right-click to cut";
  }

private:
  void createBrush(const Site& site, const Mask* mask);
  void selectPencilTool();
};

NewBrushCommand::NewBrushCommand()
  : Command("NewBrush",
            "New Brush",
            CmdUIOnlyFlag)
{
}

bool NewBrushCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void NewBrushCommand::onExecute(Context* context)
{
  ASSERT(current_editor);
  if (!current_editor)
    return;

  // If there is no visible mask, the brush must be selected from the
  // current editor.
  if (!context->activeDocument()->isMaskVisible()) {
    EditorStatePtr state = current_editor->getState();
    if (dynamic_cast<SelectBoxState*>(state.get())) {
      // If already are in "SelectBoxState" state, in this way we
      // avoid creating a stack of several "SelectBoxState" states.
      return;
    }

    current_editor->setState(
      EditorStatePtr(
        new SelectBoxState(
          this, current_editor->sprite()->bounds(),
          SelectBoxState::DARKOUTSIDE |
          SelectBoxState::QUICKBOX)));
  }
  // Create a brush from the active selection
  else {
    createBrush(context->activeSite(),
                context->activeDocument()->mask());
    selectPencilTool();

    // Deselect mask
    Command* cmd =
      CommandsModule::instance()->getCommandByName(CommandId::DeselectMask);
    UIContext::instance()->executeCommand(cmd);
  }
}

void NewBrushCommand::onQuickboxEnd(Editor* editor, const gfx::Rect& rect, ui::MouseButtons buttons)
{
  Mask mask;
  mask.replace(rect);
  createBrush(editor->getSite(), &mask);
  selectPencilTool();

  // If the right-button was used, we clear the selected area.
  if (buttons & ui::kButtonRight) {
    try {
      ContextWriter writer(UIContext::instance(), 250);
      Transaction transaction(writer.context(), "Clear");
      transaction.execute(new cmd::ClearRect(writer.cel(), rect));
      transaction.commit();
    }
    catch (const std::exception& ex) {
      Console::showException(ex);
    }
  }

  // Update the context bar
  // TODO find a way to avoid all these singletons. Maybe a simple
  // signal in the context like "brush has changed" could be enough.
  App::instance()->getMainWindow()->getContextBar()
    ->updateForCurrentTool();

  editor->backToPreviousState();
}

void NewBrushCommand::onQuickboxCancel(Editor* editor)
{
  editor->backToPreviousState();
}

void NewBrushCommand::createBrush(const Site& site, const Mask* mask)
{
  doc::ImageRef image(new_image_from_mask(site, mask));
  if (!image)
    return;

  // New brush
  doc::BrushRef brush(new doc::Brush());
  brush->setImage(image.get());
  brush->setPatternOrigin(mask->bounds().getOrigin());

  // TODO add a active stock property in app::Context
  ContextBar* ctxBar = App::instance()->getMainWindow()->getContextBar();
  int slot = ctxBar->addBrush(brush);
  ctxBar->setActiveBrush(brush);

  // Get the shortcut for this brush and show it to the user
  Params params;
  params.set("change", "custom");
  params.set("slot", base::convert_to<std::string>(slot).c_str());
  Key* key = KeyboardShortcuts::instance()->command(
    CommandId::ChangeBrush, params);
  if (key && !key->accels().empty()) {
    std::string tooltip;
    tooltip += "Shortcut: ";
    tooltip += key->accels().front().toString();
    StatusBar::instance()->showTip(2000, tooltip.c_str());
  }
}

void NewBrushCommand::selectPencilTool()
{
  if (App::instance()->activeTool()->getInk(0)->isSelection())
    Preferences::instance().toolBox.activeTool(tools::WellKnownTools::Pencil);
}

Command* CommandFactory::createNewBrushCommand()
{
  return new NewBrushCommand();
}

} // namespace app
