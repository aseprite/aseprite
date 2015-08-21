// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/set_layer_blend_mode.h"
#include "app/cmd/set_layer_name.h"
#include "app/cmd/set_layer_opacity.h"
#include "app/commands/command.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/modules/gui.h"
#include "app/transaction.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/scoped_value.h"
#include "doc/document.h"
#include "doc/document_event.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "ui/ui.h"

#include "generated_layer_properties.h"

namespace app {

using namespace ui;

class LayerPropertiesCommand : public Command {
public:
  LayerPropertiesCommand();
  Command* clone() const override { return new LayerPropertiesCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

class LayerPropertiesWindow;
static LayerPropertiesWindow* g_window = nullptr;

class LayerPropertiesWindow : public app::gen::LayerProperties
                            , public doc::ContextObserver
                            , public doc::DocumentObserver {
public:
  LayerPropertiesWindow()
    : m_timer(250, this)
    , m_layer(nullptr)
    , m_selfUpdate(false) {
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

    name()->EntryChange.connect(Bind<void>(&LayerPropertiesWindow::onShowChange, this));
    mode()->Change.connect(Bind<void>(&LayerPropertiesWindow::onShowChange, this));
    opacity()->Change.connect(Bind<void>(&LayerPropertiesWindow::onShowChange, this));
    m_timer.Tick.connect(Bind<void>(&LayerPropertiesWindow::onCommitChange, this));

    remapWindow();
    centerWindow();
    load_window_pos(this, "LayerProperties");

    UIContext::instance()->addObserver(this);
  }

  ~LayerPropertiesWindow() {
    UIContext::instance()->removeObserver(this);
  }

  void setLayer(LayerImage* layer) {
    // Save uncommited changes
    if (m_layer) {
      if (m_timer.isRunning())
        onCommitChange();

      document()->removeObserver(this);
      m_layer = nullptr;
    }

    m_timer.stop();
    m_layer = const_cast<LayerImage*>(layer);

    base::ScopedValue<bool> switchSelf(m_selfUpdate, true, false);

    if (m_layer) {
      document()->addObserver(this);

      m_oldName = layer->name();
      m_oldBlendMode = layer->blendMode();
      m_oldOpacity = layer->opacity();

      name()->setText(layer->name().c_str());
      name()->setEnabled(true);
      mode()->setSelectedItemIndex((int)layer->blendMode());
      mode()->setEnabled(!layer->isBackground());
      opacity()->setValue(layer->opacity());
      opacity()->setEnabled(!layer->isBackground());
    }
    else {
      name()->setText("No Layer");
      name()->setEnabled(false);
      mode()->setEnabled(false);
      opacity()->setEnabled(false);
    }
  }

private:

  app::Document* document() {
    ASSERT(m_layer);
    if (m_layer)
      return static_cast<app::Document*>(m_layer->sprite()->document());
    else
      return nullptr;
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

  bool onProcessMessage(ui::Message* msg) override {
    switch (msg->type()) {

      case kCloseMessage:
        // Save changes before we close the window
        setLayer(nullptr);
        save_window_pos(this, "LayerProperties");

        deferDelete();
        g_window = nullptr;
        break;

    }
    return Window::onProcessMessage(msg);
  }

  void onShowChange() {
    if (m_selfUpdate)
      return;

    m_layer->setName(nameValue());
    m_layer->setBlendMode(blendModeValue());
    m_layer->setOpacity(opacityValue());
    m_timer.start();

    update_screen_for_document(document());
  }

  void onCommitChange() {
    base::ScopedValue<bool> switchSelf(m_selfUpdate, true, false);

    m_timer.stop();

    std::string newName = nameValue();
    int newOpacity = opacityValue();
    BlendMode newBlendMode = blendModeValue();

    m_layer->setName(m_oldName);
    m_layer->setOpacity(m_oldOpacity);
    m_layer->setBlendMode(m_oldBlendMode);

    if (newName != m_oldName ||
        newOpacity != m_oldOpacity ||
        newBlendMode != m_oldBlendMode) {
      try {
        ContextWriter writer(UIContext::instance());
        Transaction transaction(writer.context(), "Set Layer Properties");

        if (newName != m_oldName)
          transaction.execute(new cmd::SetLayerName(writer.layer(), newName));

        if (newOpacity != m_oldOpacity)
          transaction.execute(new cmd::SetLayerOpacity(static_cast<LayerImage*>(writer.layer()), newOpacity));

        if (newBlendMode != m_oldBlendMode)
          transaction.execute(new cmd::SetLayerBlendMode(static_cast<LayerImage*>(writer.layer()), newBlendMode));

        transaction.commit();
      }
      catch (const std::exception& e) {
        Console::showException(e);
      }

      update_screen_for_document(document());
    }
  }

  // ContextObserver impl
  void onActiveSiteChange(const Site& site) override {
    if (isVisible())
      setLayer(dynamic_cast<LayerImage*>(const_cast<Layer*>(site.layer())));
    else if (m_layer)
      setLayer(nullptr);
  }

  // DocumentObserver impl
  void onLayerNameChange(DocumentEvent& ev) override {
    updateFromLayer(ev.layer());
  }

  void onLayerOpacityChange(DocumentEvent& ev) override {
    updateFromLayer(ev.layer());
  }

  void onLayerBlendModeChange(DocumentEvent& ev) override {
    updateFromLayer(ev.layer());
  }

  void updateFromLayer(Layer* layer) {
    if (m_selfUpdate)
      return;

    // Cancel current editions (just in case)
    m_timer.stop();
    setLayer(dynamic_cast<LayerImage*>(layer));
  }

  Timer m_timer;
  LayerImage* m_layer;
  std::string m_oldName;
  BlendMode m_oldBlendMode;
  int m_oldOpacity;
  bool m_selfUpdate;
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
  ContextReader reader(context);
  LayerImage* layer = static_cast<LayerImage*>(reader.layer());

  if (!g_window)
    g_window = new LayerPropertiesWindow;

  g_window->setLayer(layer);
  g_window->openWindow();

  // Focus layer name
  g_window->name()->requestFocus();
}

Command* CommandFactory::createLayerPropertiesCommand()
{
  return new LayerPropertiesCommand;
}

} // namespace app
