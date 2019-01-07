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
#include "app/tx.h"
#include "app/ui/editor/editor.h"
#include "app/util/expand_cel_canvas.h"
#include "doc/algorithm/fill_selection.h"
#include "doc/algorithm/stroke_selection.h"
#include "doc/mask.h"

namespace app {

class FillCommand : public Command {
public:
  enum Type { Fill, Stroke };
  FillCommand(Type type);
protected:
  bool onEnabled(Context* ctx) override;
  void onExecute(Context* ctx) override;
private:
  Type m_type;
};

FillCommand::FillCommand(Type type)
  : Command(type == Stroke ? CommandId::Stroke():
                             CommandId::Fill(), CmdUIOnlyFlag)
  , m_type(type)
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
  Doc* doc = site.document();
  Sprite* sprite = site.sprite();
  Layer* layer = site.layer();
  Mask* mask = doc->mask();
  if (!doc || !sprite ||
      !layer || !layer->isImage() ||
      !mask || !doc->isMaskVisible())
    return;

  Preferences& pref = Preferences::instance();
  app::Color color = pref.colorBar.fgColor();

  {
    Tx tx(writer.context(), "Fill Selection with Foreground Color");
    {
      ExpandCelCanvas expand(
        site, layer,
        TiledMode::NONE, tx,
        ExpandCelCanvas::None);

      gfx::Region rgn(sprite->bounds() |
                      mask->bounds());
      expand.validateDestCanvas(rgn);

      const gfx::Point offset = (mask->bounds().origin()
                                 - expand.getCel()->position());
      const doc::color_t docColor =
        color_utils::color_for_layer(
          color, layer);

      if (m_type == Stroke) {
        doc::algorithm::stroke_selection(
          expand.getDestCanvas(),
          offset,
          mask,
          docColor);
      }
      else {
        doc::algorithm::fill_selection(
          expand.getDestCanvas(),
          offset,
          mask,
          docColor);
      }

      expand.commit();
    }

    // If the cel wasn't deleted by cmd::ClearMask, we trim it.
    Cel* cel = ctx->activeSite().cel();
    if (cel && layer->isTransparent())
      tx(new cmd::TrimCel(cel));

    tx.commit();
  }

  doc->notifyGeneralUpdate();
}

Command* CommandFactory::createFillCommand()
{
  return new FillCommand(FillCommand::Fill);
}

Command* CommandFactory::createStrokeCommand()
{
  return new FillCommand(FillCommand::Stroke);
}

} // namespace app
