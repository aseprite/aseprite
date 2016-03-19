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
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

SelectTileCommand::SelectTileCommand()
  : Command("SelectTile",
            "Select Tile",
            CmdRecordableFlag)
{
}

bool SelectTileCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
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
  {
    const gfx::Rect gridBounds = docPref.grid.bounds();
    gfx::Point pos = current_editor->screenToEditor(ui::get_mouse_position());
    pos = snap_to_grid(gridBounds, pos, PreferSnapTo::BoxOrigin);

    mask->add(gfx::Rect(pos, gridBounds.size()));
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

Command* CommandFactory::createSelectTileCommand()
{
  return new SelectTileCommand;
}

} // namespace app
