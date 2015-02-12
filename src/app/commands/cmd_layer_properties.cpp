// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/bind.h"
#include "ui/ui.h"

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/modules/gui.h"
#include "doc/image.h"
#include "doc/layer.h"

#include "generated_layer_properties.h"

namespace app {

using namespace ui;

class LayerPropertiesCommand : public Command {
public:
  LayerPropertiesCommand();
  Command* clone() const override { return new LayerPropertiesCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

LayerPropertiesCommand::LayerPropertiesCommand()
  : Command("LayerProperties",
            "Layer Properties",
            CmdRecordableFlag)
{
}

bool LayerPropertiesCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveLayer);
}

void LayerPropertiesCommand::onExecute(Context* context)
{
  const ContextReader reader(context);
  const Layer* layer = reader.layer();

  app::gen::LayerProperties window;
  window.name()->setText(layer->name().c_str());
  window.name()->setMinSize(gfx::Size(128, 0));
  window.name()->setExpansive(true);
  window.openWindowInForeground();

  if (window.getKiller() == window.ok()) {
    ContextWriter writer(reader);
    writer.layer()->setName(window.name()->getText());
    update_screen_for_document(writer.document());
  }
}

Command* CommandFactory::createLayerPropertiesCommand()
{
  return new LayerPropertiesCommand;
}

} // namespace app
