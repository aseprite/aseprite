// Aseprite
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/clear_mask.h"
#include "app/cmd/trim_cel.h"
#include "app/color_utils.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/modules/editors.h"
#include "app/pref/preferences.h"
#include "app/transaction.h"
#include "app/ui/editor/editor.h"
#include "app/util/expand_cel_canvas.h"
#include "app/util/fill_selection.h"
#include "doc/mask.h"

namespace app {

class FillCommand : public Command {
public:
  FillCommand();
  Command* clone() const override { return new FillCommand(*this); }

protected:
  bool onEnabled(Context* ctx) override;
  void onExecute(Context* ctx) override;
};

FillCommand::FillCommand()
  : Command(CommandId::Fill(), CmdUIOnlyFlag)
{
}

bool FillCommand::onEnabled(Context* ctx)
{
  if (ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                      ContextFlags::ActiveLayerIsVisible |
                      ContextFlags::ActiveLayerIsEditable |
                      ContextFlags::ActiveLayerIsImage)) {
    return true;
  }
  else if (current_editor &&
           current_editor->isMovingPixels()) {
    return true;
  }
  else
    return false;
}

void FillCommand::onExecute(Context* ctx)
{
  ContextWriter writer(ctx);
  Site site = *writer.site();
  Document* doc = (app::Document*)site.document();
  Sprite* sprite = site.sprite();
  Cel* cel = site.cel();
  Mask* mask = doc->mask();
  if (!doc || !sprite || !cel || !mask ||
      !doc->isMaskVisible())
    return;

  Preferences& pref = Preferences::instance();
  app::Color color = pref.colorBar.fgColor();

  {
    Transaction transaction(writer.context(), "Fill Selection with Foreground Color");
    {
      ExpandCelCanvas expand(
        site, cel->layer(),
        TiledMode::NONE, transaction,
        ExpandCelCanvas::None);

      gfx::Region rgn(sprite->bounds() |
                      mask->bounds());
      expand.validateDestCanvas(rgn);

      const gfx::Point offset = (mask->bounds().origin()
                                 - expand.getCel()->position());

      fill_selection(expand.getDestCanvas(),
                     offset,
                     mask,
                     color_utils::color_for_layer(color,
                                                  cel->layer()));

      expand.commit();
    }

    // If the cel wasn't deleted by cmd::ClearMask, we trim it.
    cel = ctx->activeSite().cel();
    if (cel && cel->layer()->isTransparent())
      transaction.execute(new cmd::TrimCel(cel));

    transaction.commit();
  }

  doc->notifyGeneralUpdate();
}

Command* CommandFactory::createFillCommand()
{
  return new FillCommand;
}

} // namespace app
