// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/add_cel.h"
#include "app/cmd/replace_image.h"
#include "app/cmd/set_cel_position.h"
#include "app/cmd/unlink_cel.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/doc.h"
#include "app/doc_api.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "doc/blend_internals.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "render/render.h"
#include "ui/ui.h"

namespace app {

class MergeDownLayerCommand : public Command {
public:
  MergeDownLayerCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

MergeDownLayerCommand::MergeDownLayerCommand()
  : Command(CommandId::MergeDownLayer(), CmdRecordableFlag)
{
}

bool MergeDownLayerCommand::onEnabled(Context* context)
{
  if (!context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                           ContextFlags::HasActiveSprite))
    return false;

  const ContextReader reader(context);
  const Sprite* sprite(reader.sprite());
  if (!sprite)
    return false;

  const Layer* src_layer = reader.layer();
  if (!src_layer || !src_layer->isImage())
    return false;

  const Layer* dst_layer = src_layer->getPrevious();
  if (!dst_layer || !dst_layer->isImage())
    return false;

  return true;
}

void MergeDownLayerCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Doc* document(writer.document());
  Sprite* sprite(writer.sprite());
  Tx tx(writer.context(), "Merge Down Layer", ModifyDocument);
  LayerImage* src_layer = static_cast<LayerImage*>(writer.layer());
  Layer* dst_layer = src_layer->getPrevious();

  for (frame_t frpos = 0; frpos<sprite->totalFrames(); ++frpos) {
    // Get frames
    Cel* src_cel = src_layer->cel(frpos);
    Cel* dst_cel = dst_layer->cel(frpos);

    // Get images
    Image* src_image;
    if (src_cel != NULL)
      src_image = src_cel->image();
    else
      src_image = NULL;

    ImageRef dst_image;
    if (dst_cel)
      dst_image = dst_cel->imageRef();

    // With source image?
    if (src_image) {
      int t;
      int opacity;
      opacity = MUL_UN8(src_cel->opacity(), src_layer->opacity(), t);

      // No destination image
      if (!dst_image) {  // Only a transparent layer can have a null cel
        // Copy this cel to the destination layer...

        // Creating a copy of the image
        dst_image.reset(Image::createCopy(src_image));

        // Creating a copy of the cell
        dst_cel = new Cel(frpos, dst_image);
        dst_cel->setPosition(src_cel->x(), src_cel->y());
        dst_cel->setOpacity(opacity);

        tx(new cmd::AddCel(dst_layer, dst_cel));
      }
      // With destination
      else {
        gfx::Rect bounds;

        // Merge down in the background layer
        if (dst_layer->isBackground()) {
          bounds = sprite->bounds();
        }
        // Merge down in a transparent layer
        else {
          bounds = src_cel->bounds().createUnion(dst_cel->bounds());
        }

        doc::color_t bgcolor = app_get_color_to_clear_layer(dst_layer);

        ImageRef new_image(doc::crop_image(
            dst_image.get(),
            bounds.x-dst_cel->x(),
            bounds.y-dst_cel->y(),
            bounds.w, bounds.h, bgcolor));

        // Merge src_image in new_image
        render::composite_image(
          new_image.get(), src_image,
          sprite->palette(src_cel->frame()),
          src_cel->x()-bounds.x,
          src_cel->y()-bounds.y,
          opacity,
          src_layer->blendMode());

        // First unlink the dst_cel
        if (dst_cel->links())
          tx(new cmd::UnlinkCel(dst_cel));

        // Then modify the dst_cel
        tx(new cmd::SetCelPosition(dst_cel,
            bounds.x, bounds.y));

        tx(new cmd::ReplaceImage(sprite,
            dst_cel->imageRef(), new_image));
      }
    }
  }

  document->notifyLayerMergedDown(src_layer, dst_layer);
  document->getApi(tx).removeLayer(src_layer); // src_layer is deleted inside removeLayer()

  tx.commit();

#ifdef ENABLE_UI
  if (context->isUIAvailable())
    update_screen_for_document(document);
#endif
}

Command* CommandFactory::createMergeDownLayerCommand()
{
  return new MergeDownLayerCommand;
}

} // namespace app
