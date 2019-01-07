// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/set_layer_blend_mode.h"
#include "app/cmd/set_layer_name.h"
#include "app/cmd/set_layer_opacity.h"
#include "app/cmd/set_user_data.h"
#include "app/commands/command.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/doc.h"
#include "app/doc_event.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "app/ui/separator_in_view.h"
#include "app/ui/timeline/timeline.h"
#include "app/ui/user_data_popup.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/scoped_value.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "doc/user_data.h"
#include "ui/ui.h"

#include "layer_properties.xml.h"

namespace app {

using namespace ui;

class LayerPropertiesCommand : public Command {
public:
  LayerPropertiesCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

class LayerPropertiesWindow;
static LayerPropertiesWindow* g_window = nullptr;

class LayerPropertiesWindow : public app::gen::LayerProperties,
                              public ContextObserver,
                              public DocObserver {
public:
  class BlendModeItem : public ListItem {
  public:
    BlendModeItem(const std::string& name,
                  const doc::BlendMode mode)
      : ListItem(name)
      , m_mode(mode) {
    }
    doc::BlendMode mode() const { return m_mode; }
  private:
    doc::BlendMode m_mode;
  };

  LayerPropertiesWindow()
    : m_timer(250, this)
    , m_document(nullptr)
    , m_layer(nullptr)
    , m_selfUpdate(false) {
    name()->setMinSize(gfx::Size(128, 0));
    name()->setExpansive(true);

    mode()->addItem(new BlendModeItem("Normal", doc::BlendMode::NORMAL));
    mode()->addItem(new SeparatorInView);
    mode()->addItem(new BlendModeItem("Darken", doc::BlendMode::DARKEN));
    mode()->addItem(new BlendModeItem("Multiply", doc::BlendMode::MULTIPLY));
    mode()->addItem(new BlendModeItem("Color Burn", doc::BlendMode::COLOR_BURN));
    mode()->addItem(new SeparatorInView);
    mode()->addItem(new BlendModeItem("Lighten", doc::BlendMode::LIGHTEN));
    mode()->addItem(new BlendModeItem("Screen", doc::BlendMode::SCREEN));
    mode()->addItem(new BlendModeItem("Color Dodge", doc::BlendMode::COLOR_DODGE));
    mode()->addItem(new BlendModeItem("Addition", doc::BlendMode::ADDITION));
    mode()->addItem(new SeparatorInView);
    mode()->addItem(new BlendModeItem("Overlay", doc::BlendMode::OVERLAY));
    mode()->addItem(new BlendModeItem("Soft Light", doc::BlendMode::SOFT_LIGHT));
    mode()->addItem(new BlendModeItem("Hard Light", doc::BlendMode::HARD_LIGHT));
    mode()->addItem(new SeparatorInView);
    mode()->addItem(new BlendModeItem("Difference", doc::BlendMode::DIFFERENCE));
    mode()->addItem(new BlendModeItem("Exclusion", doc::BlendMode::EXCLUSION));
    mode()->addItem(new BlendModeItem("Subtract", doc::BlendMode::SUBTRACT));
    mode()->addItem(new BlendModeItem("Divide", doc::BlendMode::DIVIDE));
    mode()->addItem(new SeparatorInView);
    mode()->addItem(new BlendModeItem("Hue", doc::BlendMode::HSL_HUE));
    mode()->addItem(new BlendModeItem("Saturation", doc::BlendMode::HSL_SATURATION));
    mode()->addItem(new BlendModeItem("Color", doc::BlendMode::HSL_COLOR));
    mode()->addItem(new BlendModeItem("Luminosity", doc::BlendMode::HSL_LUMINOSITY));

    name()->Change.connect(base::Bind<void>(&LayerPropertiesWindow::onStartTimer, this));
    mode()->Change.connect(base::Bind<void>(&LayerPropertiesWindow::onStartTimer, this));
    opacity()->Change.connect(base::Bind<void>(&LayerPropertiesWindow::onStartTimer, this));
    m_timer.Tick.connect(base::Bind<void>(&LayerPropertiesWindow::onCommitChange, this));
    userData()->Click.connect(base::Bind<void>(&LayerPropertiesWindow::onPopupUserData, this));

    remapWindow();
    centerWindow();
    load_window_pos(this, "LayerProperties");

    UIContext::instance()->add_observer(this);
  }

  ~LayerPropertiesWindow() {
    UIContext::instance()->remove_observer(this);
  }

  void setLayer(Doc* doc, Layer* layer) {
    if (m_layer) {
      m_document->remove_observer(this);
      m_layer = nullptr;
    }

    m_timer.stop();
    m_document = doc;
    m_layer = layer;
    m_range = App::instance()->timeline()->range();

    if (m_document)
      m_document->add_observer(this);

    updateFromLayer();
  }

private:

  std::string nameValue() const {
    return name()->text();
  }

  BlendMode blendModeValue() const {
    BlendModeItem* item = dynamic_cast<BlendModeItem*>(mode()->getSelectedItem());
    if (item)
      return item->mode();
    else
      return doc::BlendMode::NORMAL;
  }

  int opacityValue() const {
    return opacity()->getValue();
  }

  int countLayers() const {
    if (!m_document)
      return 0;
    else if (m_layer && !m_range.enabled())
      return 1;
    else if (m_range.enabled())
      return m_range.layers();
    else
      return 0;
  }

  bool onProcessMessage(ui::Message* msg) override {
    switch (msg->type()) {

      case kKeyDownMessage:
        if (name()->hasFocus() ||
            opacity()->hasFocus() ||
            mode()->hasFocus()) {
          KeyScancode scancode = static_cast<KeyMessage*>(msg)->scancode();
          if (scancode == kKeyEnter ||
              scancode == kKeyEsc) {
            onCommitChange();
            closeWindow(this);
            return true;
          }
        }
        break;

      case kCloseMessage:
        // Save changes before we close the window
        setLayer(nullptr, nullptr);
        save_window_pos(this, "LayerProperties");

        deferDelete();
        g_window = nullptr;
        break;

    }
    return Window::onProcessMessage(msg);
  }

  void onStartTimer() {
    if (m_selfUpdate)
      return;

    m_timer.start();
  }

  void onCommitChange() {
    // Nothing to do here, as there is no layer selected.
    if (!m_layer)
      return;

    base::ScopedValue<bool> switchSelf(m_selfUpdate, true, false);

    m_timer.stop();

    const int count = countLayers();

    std::string newName = nameValue();
    int newOpacity = opacityValue();
    BlendMode newBlendMode = blendModeValue();

    if ((count > 1) ||
        (count == 1 && m_layer && (newName != m_layer->name() ||
                                   m_userData != m_layer->userData() ||
                                   (m_layer->isImage() &&
                                    (newOpacity != static_cast<LayerImage*>(m_layer)->opacity() ||
                                     newBlendMode != static_cast<LayerImage*>(m_layer)->blendMode()))))) {
      try {
        ContextWriter writer(UIContext::instance());
        Tx tx(writer.context(), "Set Layer Properties");

        DocRange range;
        if (m_range.enabled())
          range = m_range;
        else {
          range.startRange(m_layer, -1, DocRange::kLayers);
          range.endRange(m_layer, -1);
        }

        const bool nameChanged = (newName != m_layer->name());
        const bool userDataChanged = (m_userData != m_layer->userData());
        const bool opacityChanged = (m_layer->isImage() && newOpacity != static_cast<LayerImage*>(m_layer)->opacity());
        const bool blendModeChanged = (m_layer->isImage() && newBlendMode != static_cast<LayerImage*>(m_layer)->blendMode());

        for (Layer* layer : range.selectedLayers()) {
          if (nameChanged && newName != layer->name())
            tx(new cmd::SetLayerName(layer, newName));

          if (userDataChanged && m_userData != layer->userData())
            tx(new cmd::SetUserData(layer, m_userData));

          if (layer->isImage()) {
            if (opacityChanged && newOpacity != static_cast<LayerImage*>(layer)->opacity())
              tx(new cmd::SetLayerOpacity(static_cast<LayerImage*>(layer), newOpacity));

            if (blendModeChanged && newBlendMode != static_cast<LayerImage*>(layer)->blendMode())
              tx(new cmd::SetLayerBlendMode(static_cast<LayerImage*>(layer), newBlendMode));
          }
        }

        // Redraw timeline because the layer's name/user data/color
        // might have changed.
        App::instance()->timeline()->invalidate();

        tx.commit();
      }
      catch (const std::exception& e) {
        Console::showException(e);
      }

      update_screen_for_document(m_document);
    }
  }

  // ContextObserver impl
  void onActiveSiteChange(const Site& site) override {
    if (isVisible())
      setLayer(const_cast<Doc*>(site.document()),
               const_cast<Layer*>(site.layer()));
    else if (m_layer)
      setLayer(nullptr, nullptr);
  }

  // DocObserver impl
  void onLayerNameChange(DocEvent& ev) override {
    if (m_layer == ev.layer())
      updateFromLayer();
  }

  void onLayerOpacityChange(DocEvent& ev) override {
    if (m_layer == ev.layer())
      updateFromLayer();
  }

  void onLayerBlendModeChange(DocEvent& ev) override {
    if (m_layer == ev.layer())
      updateFromLayer();
  }

  void onPopupUserData() {
    if (m_layer) {
      m_userData = m_layer->userData();
      if (show_user_data_popup(userData()->bounds(), m_userData)) {
        onCommitChange();
      }
    }
  }

  void updateFromLayer() {
    if (m_selfUpdate)
      return;

    m_timer.stop(); // Cancel current editions (just in case)

    base::ScopedValue<bool> switchSelf(m_selfUpdate, true, false);

    if (m_layer) {
      name()->setText(m_layer->name().c_str());
      name()->setEnabled(true);

      if (m_layer->isImage()) {
        mode()->setSelectedItem(nullptr);
        for (auto item : *mode()) {
          if (auto blendModeItem = dynamic_cast<BlendModeItem*>(item)) {
            if (blendModeItem->mode() == static_cast<LayerImage*>(m_layer)->blendMode()) {
              mode()->setSelectedItem(item);
              break;
            }
          }
        }
        mode()->setEnabled(!m_layer->isBackground());
        opacity()->setValue(static_cast<LayerImage*>(m_layer)->opacity());
        opacity()->setEnabled(!m_layer->isBackground());
      }
      else {
        mode()->setEnabled(false);
        opacity()->setEnabled(false);
      }

      m_userData = m_layer->userData();
    }
    else {
      name()->setText("No Layer");
      name()->setEnabled(false);
      mode()->setEnabled(false);
      opacity()->setEnabled(false);
      m_userData = UserData();
    }
  }

  Timer m_timer;
  Doc* m_document;
  Layer* m_layer;
  DocRange m_range;
  bool m_selfUpdate;
  UserData m_userData;
};

LayerPropertiesCommand::LayerPropertiesCommand()
  : Command(CommandId::LayerProperties(), CmdRecordableFlag)
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
  Doc* doc = static_cast<Doc*>(reader.document());
  LayerImage* layer = static_cast<LayerImage*>(reader.layer());

  if (!g_window)
    g_window = new LayerPropertiesWindow;

  g_window->setLayer(doc, layer);
  g_window->openWindow();

  // Focus layer name
  g_window->name()->requestFocus();
}

Command* CommandFactory::createLayerPropertiesCommand()
{
  return new LayerPropertiesCommand;
}

} // namespace app
