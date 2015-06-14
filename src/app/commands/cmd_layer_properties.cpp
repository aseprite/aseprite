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
#include "app/cmd/set_layer_opacity.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/modules/gui.h"
#include "app/transaction.h"
#include "doc/document.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"

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

class LayerPropertiesWindow : public app::gen::LayerProperties {
public:

  LayerPropertiesWindow(const LayerImage* layer)
    : m_layer(const_cast<LayerImage*>(layer))
    , m_oldBlendMode(layer->blendMode())
    , m_oldOpacity(layer->opacity()) {
    name()->setText(layer->name().c_str());
    name()->setMinSize(gfx::Size(128, 0));
    name()->setExpansive(true);

    mode()->addItem("Normal");
    mode()->addItem("Multiply");
    mode()->addItem("Screen");
    mode()->addItem("Overlay");
    mode()->addItem("Darken");
    mode()->addItem("Lighten");
    mode()->addItem("Color Dodge");
    mode()->addItem("Color Burn");
    mode()->addItem("Hard Light");
    mode()->addItem("Soft Light");
    mode()->addItem("Difference");
    mode()->addItem("Exclusion");
    mode()->addItem("Hue");
    mode()->addItem("Saturation");
    mode()->addItem("Color");
    mode()->addItem("Luminosity");
    mode()->setSelectedItemIndex((int)layer->blendMode());
    opacity()->setValue(layer->opacity());

    mode()->setEnabled(!layer->isBackground());
    opacity()->setEnabled(!layer->isBackground());

    mode()->Change.connect(Bind<void>(&LayerPropertiesWindow::onLayerPropsChange, this));
    opacity()->Change.connect(Bind<void>(&LayerPropertiesWindow::onLayerPropsChange, this));

    remapWindow();
    centerWindow();
    load_window_pos(this, "LayerProperties");
  }

  std::string nameValue() const {
    return name()->getText();
  }

  BlendMode blendModeValue() const {
    return (BlendMode)mode()->getSelectedItemIndex();
  }

  int opacityValue() const {
    return opacity()->getValue();
  }

protected:

  bool onProcessMessage(ui::Message* msg) override {
    switch (msg->type()) {
      case kCloseMessage:
        m_layer->setBlendMode(m_oldBlendMode);
        m_layer->setOpacity(m_oldOpacity);
        save_window_pos(this, "LayerProperties");
        break;
    }
    return Window::onProcessMessage(msg);
  }

  void onLayerPropsChange() {
    m_layer->setBlendMode(blendModeValue());
    m_layer->setOpacity(opacityValue());

    update_screen_for_document(
      static_cast<app::Document*>(m_layer->sprite()->document()));
  }

  LayerImage* m_layer;
  BlendMode m_oldBlendMode;
  int m_oldOpacity;
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

  LayerPropertiesWindow window(layer);

  window.openWindowInForeground();

  if (window.getKiller() == window.ok()) {
    std::string newName = window.nameValue();
    int newOpacity = window.opacityValue();
    BlendMode newBlendMode = window.blendModeValue();

    if (newName != layer->name() ||
        newOpacity != layer->opacity() ||
        newBlendMode != layer->blendMode()) {
      ContextWriter writer(reader);
      {
        Transaction transaction(writer.context(), "Set Layer Properties");

        if (newName != layer->name())
          transaction.execute(new cmd::SetLayerName(writer.layer(), newName));

        if (newOpacity != layer->opacity())
          transaction.execute(new cmd::SetLayerOpacity(static_cast<LayerImage*>(writer.layer()), newOpacity));

        if (newBlendMode != layer->blendMode())
          transaction.execute(new cmd::SetLayerBlendMode(static_cast<LayerImage*>(writer.layer()), newBlendMode));

        transaction.commit();
      }
    }
  }

  update_screen_for_document(reader.document());
}

Command* CommandFactory::createLayerPropertiesCommand()
{
  return new LayerPropertiesCommand;
}

} // namespace app
