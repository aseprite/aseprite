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
#include "app/cmd/set_layer_blend_mode.h"
#include "app/cmd/set_layer_name.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/modules/gui.h"
#include "app/transaction.h"
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
  const LayerImage* layer = static_cast<const LayerImage*>(reader.layer());

  app::gen::LayerProperties window;

  window.name()->setText(layer->name().c_str());
  window.name()->setMinSize(gfx::Size(128, 0));
  window.name()->setExpansive(true);

  window.mode()->addItem("Normal");
  window.mode()->addItem("Multiply");
  window.mode()->addItem("Screen");
  window.mode()->addItem("Overlay");
  window.mode()->addItem("Darken");
  window.mode()->addItem("Lighten");
  window.mode()->addItem("Color Dodge");
  window.mode()->addItem("Color Burn");
  window.mode()->addItem("Hard Light");
  window.mode()->addItem("Soft Light");
  window.mode()->addItem("Difference");
  window.mode()->addItem("Exclusion");
  window.mode()->addItem("Hue");
  window.mode()->addItem("Saturation");
  window.mode()->addItem("Color");
  window.mode()->addItem("Luminosity");
  window.mode()->setSelectedItemIndex((int)layer->blendMode());
  window.mode()->setEnabled(!layer->isBackground());

  window.openWindowInForeground();

  if (window.getKiller() == window.ok()) {
    std::string newName = window.name()->getText();
    BlendMode newBlendMode = (BlendMode)window.mode()->getSelectedItemIndex();

    if (newName != layer->name() ||
        newBlendMode != layer->blendMode()) {
      ContextWriter writer(reader);
      {
        Transaction transaction(writer.context(), "Set Layer Properties");

        if (newName != layer->name())
          transaction.execute(new cmd::SetLayerName(writer.layer(), newName));

        if (newBlendMode != layer->blendMode())
          transaction.execute(new cmd::SetLayerBlendMode(static_cast<LayerImage*>(writer.layer()), newBlendMode));

        transaction.commit();
      }
      update_screen_for_document(writer.document());
    }
  }
}

Command* CommandFactory::createLayerPropertiesCommand()
{
  return new LayerPropertiesCommand;
}

} // namespace app
