// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/set_mask.h"
#include "app/color_picker.h"
#include "app/color_utils.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/tools/tool_box.h"
#include "app/tx.h"
#include "app/ui/editor/editor.h"
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

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

MaskContentCommand::MaskContentCommand()
  : Command(CommandId::MaskContent(), CmdRecordableFlag)
{
}

bool MaskContentCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::ActiveLayerIsImage);
}

void MaskContentCommand::onExecute(Context* context)
{
  Doc* document;
  {
    ContextWriter writer(context);
    document = writer.document();

    Cel* cel = writer.cel(); // Get current cel (can be NULL)
    if (!cel)
      return;

    gfx::Color color;
    if (writer.layer()->isBackground()) {
      ColorPicker picker;
      picker.pickColor(*writer.site(),
                       gfx::PointF(0.0, 0.0),
                       current_editor->projection(),
                       ColorPicker::FromComposition);
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

    Tx tx(writer.context(), "Select Content", DoesntModifyDocument);
    tx(new cmd::SetMask(document, &newMask));
    document->resetTransformation();
    tx.commit();
  }

  // Select marquee tool
  if (tools::Tool* tool = App::instance()->toolBox()
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
