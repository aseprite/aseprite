// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2015-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_mask.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/doc.h"
#include "app/i18n/strings.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/snap_to_grid.h"
#include "app/tx.h"
#include "app/ui/editor/editor.h"
#include "doc/mask.h"
#include "fmt/format.h"
#include "ui/system.h"

namespace app {

using namespace doc;

class SelectTileCommand : public Command {
public:
  SelectTileCommand();

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* ctx) override;
  void onExecute(Context* ctx) override;
  std::string onGetFriendlyName() const override;

private:
  gen::SelectionMode m_mode;
};

SelectTileCommand::SelectTileCommand()
  : Command(CommandId::SelectTile(), CmdRecordableFlag)
  , m_mode(gen::SelectionMode::DEFAULT)
{
}

void SelectTileCommand::onLoadParams(const Params& params)
{
  std::string mode = params.get("mode");
  if (mode == "add")
    m_mode = gen::SelectionMode::ADD;
  else if (mode == "subtract")
    m_mode = gen::SelectionMode::SUBTRACT;
  else if (mode == "intersect")
    m_mode = gen::SelectionMode::INTERSECT;
  else
    m_mode = gen::SelectionMode::DEFAULT;
}

bool SelectTileCommand::onEnabled(Context* ctx)
{
  return ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void SelectTileCommand::onExecute(Context* ctx)
{
  if (!current_editor ||
      !current_editor->hasMouse())
    return;

  // Lock sprite
  ContextWriter writer(ctx);
  Doc* doc(writer.document());
  auto& docPref = Preferences::instance().document(doc);

  std::unique_ptr<Mask> mask(new Mask());

  if (m_mode != gen::SelectionMode::DEFAULT)
    mask->copyFrom(doc->mask());

  {
    gfx::Rect gridBounds = docPref.grid.bounds();
    gfx::Point pos = current_editor->screenToEditor(ui::get_mouse_position());
    pos = snap_to_grid(gridBounds, pos, PreferSnapTo::BoxOrigin);
    gridBounds.setOrigin(pos);

    switch (m_mode) {
      case gen::SelectionMode::DEFAULT:
      case gen::SelectionMode::ADD:
        mask->add(gridBounds);
        break;
      case gen::SelectionMode::SUBTRACT:
        mask->subtract(gridBounds);
        break;
      case gen::SelectionMode::INTERSECT:
        mask->intersect(gridBounds);
        break;
    }
  }

  // Set the new mask
  Tx tx(writer.context(),
                          friendlyName(),
                          DoesntModifyDocument);
  tx(new cmd::SetMask(doc, mask.get()));
  tx.commit();

  doc->generateMaskBoundaries();
  update_screen_for_document(doc);
}

std::string SelectTileCommand::onGetFriendlyName() const
{
  std::string text;
  switch (m_mode) {
    case gen::SelectionMode::ADD:
      text = Strings::commands_SelectTile_Add();
      break;
    case gen::SelectionMode::SUBTRACT:
      text = Strings::commands_SelectTile_Subtract();
      break;
    case gen::SelectionMode::INTERSECT:
      text = Strings::commands_SelectTile_Intersect();
      break;
    default:
      text = getBaseFriendlyName();
      break;
  }
  return text;
}

Command* CommandFactory::createSelectTileCommand()
{
  return new SelectTileCommand;
}

} // namespace app
