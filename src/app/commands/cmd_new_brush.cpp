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
#include "app/settings/settings.h"
#include "app/tools/tool_box.h"
#include "app/transaction.h"
#include "app/ui/context_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/select_box_state.h"
#include "app/ui/editor/tool_loop_impl.h"
#include "app/ui/main_window.h"
#include "app/ui_context.h"
#include "app/util/new_image_from_mask.h"
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
  void onQuickboxEnd(const gfx::Rect& rect, ui::MouseButtons buttons) override;
  void onQuickboxCancel() override;

private:
  void createBrush(const Mask* mask);
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
  // If there is no visible mask, the brush must be selected from the
  // current editor.
  if (!context->activeDocument()->isMaskVisible()) {
    current_editor->setState(
      EditorStatePtr(
        new SelectBoxState(
          this, current_editor->sprite()->bounds(),
          SelectBoxState::DARKOUTSIDE |
          SelectBoxState::QUICKBOX)));
  }
  // Create a brush from the active selection
  else {
    createBrush(context->activeDocument()->mask());

    // Set pencil as current tool
    ISettings* settings = UIContext::instance()->settings();
    tools::Tool* pencil =
      App::instance()->getToolBox()->getToolById(tools::WellKnownTools::Pencil);
    settings->setCurrentTool(pencil);

    // Deselect mask
    Command* cmd =
      CommandsModule::instance()->getCommandByName(CommandId::DeselectMask);
    UIContext::instance()->executeCommand(cmd);
  }
}

void NewBrushCommand::onQuickboxEnd(const gfx::Rect& rect, ui::MouseButtons buttons)
{
  Mask mask;
  mask.replace(rect);
  createBrush(&mask);

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
    ->updateFromTool(UIContext::instance()->settings()->getCurrentTool());

  current_editor->backToPreviousState();
}

void NewBrushCommand::onQuickboxCancel()
{
  current_editor->backToPreviousState();
}

void NewBrushCommand::createBrush(const Mask* mask)
{
  doc::ImageRef image(new_image_from_mask(
                        UIContext::instance()->activeSite(), mask));
  if (!image)
    return;

  // Set brush
  set_tool_loop_brush_image(
    image.get(), mask->bounds().getOrigin());
}

Command* CommandFactory::createNewBrushCommand()
{
  return new NewBrushCommand();
}

} // namespace app
