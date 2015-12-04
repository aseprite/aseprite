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
#include "app/cmd/set_mask.h"
#include "app/color_picker.h"
#include "app/color_utils.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/modules/gui.h"
#include "app/tools/tool_box.h"
#include "app/transaction.h"
#include "app/ui/toolbar.h"
#include "doc/algorithm/shrink_bounds.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/sprite.h"

namespace app {

class MaskContentCommand : public Command {
public:
  MaskContentCommand();
  Command* clone() const override { return new MaskContentCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
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
  Document* document;
  {
    ContextWriter writer(context);
    document = writer.document();

    Cel* cel = writer.cel(); // Get current cel (can be NULL)
    if (!cel)
      return;

    gfx::Color color;
    if (writer.layer()->isBackground()) {
      ColorPicker picker;
      picker.pickColor(*writer.site(), gfx::Point(0, 0), ColorPicker::FromComposition);
      color = color_utils::color_for_layer(picker.color(), writer.layer());
    }
    else
      color = cel->image()->maskColor();

    Mask newMask;
    gfx::Rect imgBounds = cel->image()->bounds();
    if (algorithm::shrink_bounds(cel->image(), imgBounds, color)) {
      newMask.replace(imgBounds.offset(cel->bounds().origin()));
    }
    else {
      newMask.replace(cel->bounds());
    }

    Transaction transaction(writer.context(), "Select Content", DoesntModifyDocument);
    transaction.execute(new cmd::SetMask(document, &newMask));
    transaction.commit();

    document->resetTransformation();
    document->generateMaskBoundaries();
  }

  // Select marquee tool
  if (tools::Tool* tool = App::instance()->getToolBox()
      ->getToolById(tools::WellKnownTools::RectangularMarquee)) {
    ToolBar::instance()->selectTool(tool);
  }

  update_screen_for_document(document);
}

Command* CommandFactory::createMaskContentCommand()
{
  return new MaskContentCommand;
}

} // namespace app
