// Aseprite
// Copyright (C) 2015, 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_mask.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/document.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/snap_to_grid.h"
#include "app/transaction.h"
#include "app/ui/editor/editor.h"
#include "doc/mask.h"
#include "ui/system.h"

namespace app {

using namespace doc;

class SelectTileCommand : public Command {
public:
  SelectTileCommand();
  Command* clone() const override { return new SelectTileCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* ctx) override;
  void onExecute(Context* ctx) override;
  std::string onGetFriendlyName() const override;

private:
  gen::SelectionMode m_mode;
};

SelectTileCommand::SelectTileCommand()
  : Command("SelectTile",
            "Select Tile",
            CmdRecordableFlag)
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
  Document* doc(writer.document());
  auto& docPref = Preferences::instance().document(doc);

  base::UniquePtr<Mask> mask(new Mask());

  if (m_mode != gen::SelectionMode::DEFAULT)
    mask->copyFrom(doc->mask());

  {
    gfx::Rect gridBounds = docPref.grid.bounds();
    gfx::Point pos = current_editor->screenToEditor(ui::get_mouse_position());
    pos = snap_to_grid(gridBounds, pos, PreferSnapTo::BoxOrigin);
    gridBounds.setOrigin(pos);

    if (m_mode != gen::SelectionMode::SUBTRACT)
      mask->add(gridBounds);
    else
      mask->subtract(gridBounds);
  }

  // Set the new mask
  Transaction transaction(writer.context(),
                          "Select Tile",
                          DoesntModifyDocument);
  transaction.execute(new cmd::SetMask(doc, mask));
  transaction.commit();

  doc->generateMaskBoundaries();
  update_screen_for_document(doc);
}

std::string SelectTileCommand::onGetFriendlyName() const
{
  std::string text = "Select Tile";

  switch (m_mode) {
    case gen::SelectionMode::ADD: text += " (Add)"; break;
    case gen::SelectionMode::SUBTRACT: text += " (Subtract)"; break;
  }

  return text;
}

Command* CommandFactory::createSelectTileCommand()
{
  return new SelectTileCommand;
}

} // namespace app
