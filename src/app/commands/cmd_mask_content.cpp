/* Aseprite
 * Copyright (C) 2001-2015  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/set_mask.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/tools/tool_box.h"
#include "app/transaction.h"
#include "app/ui/editor/editor.h"
#include "app/ui/toolbar.h"
#include "doc/algorithm/shrink_bounds.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/mask.h"
#include "doc/sprite.h"

namespace app {

class MaskContentCommand : public Command {
public:
  MaskContentCommand();
  Command* clone() const override { return new MaskContentCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

MaskContentCommand::MaskContentCommand()
  : Command("MaskContent",
            "Mask Content",
            CmdRecordableFlag)
{
}

bool MaskContentCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::ActiveLayerIsImage);
}

void MaskContentCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  Cel* cel = writer.cel(); // Get current cel (can be NULL)
  if (!cel)
    return;

  Mask newMask;
  gfx::Rect imgBounds = cel->image()->bounds();
  if (algorithm::shrink_bounds(cel->image(), imgBounds,
        cel->image()->maskColor())) {

    newMask.copyFrom(document->mask());
    newMask.replace(imgBounds.offset(cel->bounds().getOrigin()));
  }

  Transaction transaction(writer.context(), "Select Content", DoesntModifyDocument);
  transaction.execute(new cmd::SetMask(document, &newMask));
  transaction.commit();

  document->resetTransformation();
  document->generateMaskBoundaries();

  // Select marquee tool
  if (tools::Tool* tool = App::instance()->getToolBox()
        ->getToolById(tools::WellKnownTools::RectangularMarquee)) {
    ToolBar::instance()->selectTool(tool);
    current_editor->startSelectionTransformation(gfx::Point(0, 0));
  }
  
  update_screen_for_document(document);
}

Command* CommandFactory::createMaskContentCommand()
{
  return new MaskContentCommand;
}

} // namespace app
