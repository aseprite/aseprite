// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/modules/gui.h"
#include "app/ui/timeline/timeline.h"
#include "doc/image.h"
#include "doc/layer.h"

namespace app {

using namespace ui;

class LayerLockCommand : public Command {
public:
  LayerLockCommand();

protected:
  bool onEnabled(Context* context) override;
  bool onChecked(Context* context) override;
  void onExecute(Context* context) override;
};

LayerLockCommand::LayerLockCommand()
  : Command(CommandId::LayerLock(), CmdRecordableFlag)
{
}

bool LayerLockCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveLayer);
}

bool LayerLockCommand::onChecked(Context* context)
{
  const ContextReader reader(context);
  if (!reader.document() ||
      !reader.layer())
    return false;

  SelectedLayers selLayers;
  auto range = App::instance()->timeline()->range();
  if (range.enabled()) {
    selLayers = range.selectedLayers();
  }
  else {
    selLayers.insert(const_cast<Layer*>(reader.layer()));
  }
  bool lock = false;
  for (auto layer : selLayers) {
    if (layer && !layer->isEditable())
      lock = true;
  }
  return lock;
}

void LayerLockCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  SelectedLayers selLayers;
  auto range = App::instance()->timeline()->range();
  if (range.enabled()) {
    selLayers = range.selectedLayers();
  }
  else {
    selLayers.insert(writer.layer());
  }
  bool anyLock = false;
  for (auto layer : selLayers) {
    if (!layer->isEditable())
      anyLock = true;
  }
  for (auto layer : selLayers) {
    layer->setEditable(anyLock);
  }

  update_screen_for_document(writer.document());
}

Command* CommandFactory::createLayerLockCommand()
{
  return new LayerLockCommand;
}

} // namespace app
