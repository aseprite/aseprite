// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/modules/gui.h"
#include "app/ui/timeline/timeline.h"
#include "doc/image.h"
#include "doc/layer.h"

namespace app {

using namespace ui;

class LayerVisibilityCommand : public Command {
public:
  LayerVisibilityCommand();

protected:
  bool onEnabled(Context* context) override;
  bool onChecked(Context* context) override;
  void onExecute(Context* context) override;
};

LayerVisibilityCommand::LayerVisibilityCommand()
  : Command(CommandId::LayerVisibility(), CmdRecordableFlag)
{
}

bool LayerVisibilityCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable | ContextFlags::HasActiveLayer);
}

bool LayerVisibilityCommand::onChecked(Context* context)
{
  const ContextReader reader(context);
  if (!reader.document() || !reader.layer())
    return false;

  SelectedLayers selLayers;
  DocRange range = context->activeSite().range();
  if (range.enabled()) {
    selLayers = range.selectedLayers();
  }
  else {
    selLayers.insert(const_cast<Layer*>(reader.layer()));
  }
  bool visible = false;
  for (auto layer : selLayers) {
    if (layer && layer->isVisible())
      visible = true;
  }
  return visible;
}

void LayerVisibilityCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Doc* doc = writer.document();
  SelectedLayers selLayers;
  DocRange range = context->activeSite().range();
  if (range.enabled()) {
    selLayers = range.selectedLayers();
  }
  else {
    selLayers.insert(writer.layer());
  }

  bool anyVisible = false;
  for (auto layer : selLayers) {
    if (layer->isVisible())
      anyVisible = true;
  }

  const bool newState = !anyVisible;
  for (auto layer : selLayers)
    doc->setLayerVisibilityWithNotifications(layer, newState);
}

Command* CommandFactory::createLayerVisibilityCommand()
{
  return new LayerVisibilityCommand;
}

} // namespace app
