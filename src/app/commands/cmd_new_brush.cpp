// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

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
#include "app/tools/active_tool.h"
#include "app/tools/ink.h"
#include "app/tools/tool_box.h"
#include "app/tx.h"
#include "app/ui/context_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/select_box_state.h"
#include "app/ui/keyboard_shortcuts.h"
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

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;

  // SelectBoxDelegate impl
  void onQuickboxEnd(Editor* editor, const gfx::Rect& rect, ui::MouseButton button) override;
  void onQuickboxCancel(Editor* editor) override;

  std::string onGetContextBarHelp() override {
    return "Select brush bounds | Right-click to cut";
  }

private:
  void createBrush(const Site& site, const Mask* mask);
  void selectPencilTool();
};

NewBrushCommand::NewBrushCommand()
  : Command(CommandId::NewBrush(), CmdUIOnlyFlag)
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
          SelectBoxState::Flags(
            int(SelectBoxState::Flags::DarkOutside) |
            int(SelectBoxState::Flags::QuickBox)))));
  }
  // Create a brush from the active selection
  else {
    createBrush(context->activeSite(),
                context->activeDocument()->mask());
    selectPencilTool();

    // Deselect mask
    Command* cmd =
      Commands::instance()->byId(CommandId::DeselectMask());
    UIContext::instance()->executeCommand(cmd);
  }
}

void NewBrushCommand::onQuickboxEnd(Editor* editor, const gfx::Rect& rect, ui::MouseButton button)
{
  Mask mask;
  mask.replace(rect);
  createBrush(editor->getSite(), &mask);
  selectPencilTool();

  // If the right-button was used, we clear the selected area.
  if (button == ui::kButtonRight) {
    try {
      ContextWriter writer(UIContext::instance());
      if (writer.cel()) {
        gfx::Rect canvasRect = (rect & writer.cel()->bounds());
        if (!canvasRect.isEmpty()) {
          Tx tx(writer.context(), "Clear");
          tx(new cmd::ClearRect(writer.cel(), canvasRect));
          tx.commit();
        }
      }
    }
    catch (const std::exception& ex) {
      Console::showException(ex);
    }
  }

  // Update the context bar
  // TODO find a way to avoid all these singletons. Maybe a simple
  // signal in the context like "brush has changed" could be enough.
  App::instance()->contextBar()->updateForActiveTool();

  editor->backToPreviousState();
}

void NewBrushCommand::onQuickboxCancel(Editor* editor)
{
  editor->backToPreviousState();
}

void NewBrushCommand::createBrush(const Site& site, const Mask* mask)
{
  doc::ImageRef image(new_image_from_mask(site, mask,
                                          Preferences::instance().experimental.newBlend()));
  if (!image)
    return;

  // New brush
  doc::BrushRef brush(new doc::Brush());
  brush->setImage(image.get(), mask->bitmap());
  brush->setPatternOrigin(mask->bounds().origin());

  ContextBar* ctxBar = App::instance()->contextBar();
  int flags = int(BrushSlot::Flags::BrushType);
  {
    // TODO merge this code with ContextBar::createBrushSlotFromPreferences()?
    auto& pref = Preferences::instance();
    auto& saveBrush = pref.saveBrush;
    if (saveBrush.imageColor())
      flags |= int(BrushSlot::Flags::ImageColor);
  }

  int slot = App::instance()->brushes().addBrushSlot(
    BrushSlot(BrushSlot::Flags(flags), brush));
  ctxBar->setActiveBrush(brush);

  // Get the shortcut for this brush and show it to the user
  Params params;
  params.set("change", "custom");
  params.set("slot", base::convert_to<std::string>(slot).c_str());
  KeyPtr key = KeyboardShortcuts::instance()->command(
    CommandId::ChangeBrush(), params);
  if (key && !key->accels().empty()) {
    std::string tooltip;
    tooltip += "Shortcut: ";
    tooltip += key->accels().front().toString();
    StatusBar::instance()->showTip(2000, tooltip);
  }
}

void NewBrushCommand::selectPencilTool()
{
  App* app = App::instance();
  if (app->activeToolManager()->selectedTool()->getInk(0)->isSelection()) {
    app->activeToolManager()->setSelectedTool(
      app->toolBox()->getToolById(tools::WellKnownTools::Pencil));
  }
}

Command* CommandFactory::createNewBrushCommand()
{
  return new NewBrushCommand();
}

} // namespace app
