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
#include "app/cmd/flatten_layers.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/doc.h"
#include "app/doc_api.h"
#include "app/doc_range.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/tx.h"
#include "doc/blend_internals.h"
#include "doc/layer.h"
#include "doc/sprite.h"
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
  if (!context->checkFlags(ContextFlags::ActiveDocumentIsWritable | ContextFlags::HasActiveSprite))
    return false;

  const ContextReader reader(context);
  const Sprite* sprite(reader.sprite());
  if (!sprite)
    return false;

  const Layer* src_layer = reader.layer();
  if (!src_layer || !src_layer->isImage() || src_layer->isTilemap()) // TODO Add support to merge
                                                                     // tilemaps (and groups!)
    return false;

  const Layer* dst_layer = src_layer->getPrevious();
  if (!dst_layer || !dst_layer->isImage() || dst_layer->isTilemap()) // TODO Add support to merge
                                                                     // tilemaps
    return false;

  return true;
}

void MergeDownLayerCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Doc* document(writer.document());
  Sprite* sprite(writer.sprite());
  LayerImage* src_layer = static_cast<LayerImage*>(writer.layer());
  Layer* dst_layer = src_layer->getPrevious();

  Tx tx(writer, friendlyName(), ModifyDocument);

  DocRange range;
  range.selectLayer(writer.layer());
  range.selectLayer(dst_layer);

  const bool newBlend = Preferences::instance().experimental.newBlend();
  cmd::FlattenLayers::Options options;
  options.newBlendMethod = newBlend;
  options.inplace = true;
  options.mergeDown = true;
  options.dynamicCanvas = true;

  tx(new cmd::FlattenLayers(sprite, range.selectedLayers(), options));
  tx.commit();

  update_screen_for_document(document);
}

Command* CommandFactory::createMergeDownLayerCommand()
{
  return new MergeDownLayerCommand;
}

} // namespace app
