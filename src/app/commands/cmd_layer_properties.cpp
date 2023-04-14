// Aseprite
// Copyright (C) 2020-2022  Igara Studio S.A.
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
#include "app/cmd/set_tileset_base_index.h"
#include "app/cmd/set_tileset_name.h"
#include "app/cmd/set_user_data.h"
#include "app/commands/command.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/doc.h"
#include "app/doc_event.h"
#include "app/i18n/strings.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "app/ui/main_window.h"
#include "app/ui/separator_in_view.h"
#include "app/ui/tileset_selector.h"
#include "app/ui/timeline/timeline.h"
#include "app/ui/user_data_view.h"
#include "app/ui_context.h"
#include "base/scoped_value.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/layer_tilemap.h"
#include "doc/sprite.h"
#include "doc/tileset.h"
#include "doc/user_data.h"
#include "ui/ui.h"

#include "layer_properties.xml.h"
#include "tileset_selector_window.xml.h"

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
    , m_userDataView(Preferences::instance().layers.userDataVisibility) {
    name()->setMinSize(gfx::Size(128, 0));
    name()->setExpansive(true);

    mode()->addItem(new BlendModeItem(Strings::layer_properties_normal(),
                                      doc::BlendMode::NORMAL));
    mode()->addItem(new SeparatorInView);
    mode()->addItem(new BlendModeItem(Strings::layer_properties_darken(),
                                      doc::BlendMode::DARKEN));
    mode()->addItem(new BlendModeItem(Strings::layer_properties_multiply(),
                                      doc::BlendMode::MULTIPLY));
    mode()->addItem(new BlendModeItem(Strings::layer_properties_color_burn(),
                                      doc::BlendMode::COLOR_BURN));
    mode()->addItem(new SeparatorInView);
    mode()->addItem(new BlendModeItem(Strings::layer_properties_lighten(),
                                      doc::BlendMode::LIGHTEN));
    mode()->addItem(new BlendModeItem(Strings::layer_properties_screen(),
                                      doc::BlendMode::SCREEN));
    mode()->addItem(new BlendModeItem(Strings::layer_properties_color_dodge(),
                                      doc::BlendMode::COLOR_DODGE));
    mode()->addItem(new BlendModeItem(Strings::layer_properties_addition(),
                                      doc::BlendMode::ADDITION));
    mode()->addItem(new SeparatorInView);
    mode()->addItem(new BlendModeItem(Strings::layer_properties_overlay(),
                                      doc::BlendMode::OVERLAY));
    mode()->addItem(new BlendModeItem(Strings::layer_properties_soft_light(),
                                      doc::BlendMode::SOFT_LIGHT));
    mode()->addItem(new BlendModeItem(Strings::layer_properties_hard_light(),
                                      doc::BlendMode::HARD_LIGHT));
    mode()->addItem(new SeparatorInView);
    mode()->addItem(new BlendModeItem(Strings::layer_properties_difference(),
                                      doc::BlendMode::DIFFERENCE));
    mode()->addItem(new BlendModeItem(Strings::layer_properties_exclusion(),
                                      doc::BlendMode::EXCLUSION));
    mode()->addItem(new BlendModeItem(Strings::layer_properties_subtract(),
                                      doc::BlendMode::SUBTRACT));
    mode()->addItem(new BlendModeItem(Strings::layer_properties_divide(),
                                      doc::BlendMode::DIVIDE));
    mode()->addItem(new SeparatorInView);
    mode()->addItem(new BlendModeItem(Strings::layer_properties_hue(),
                                      doc::BlendMode::HSL_HUE));
    mode()->addItem(new BlendModeItem(Strings::layer_properties_saturation(),
                                      doc::BlendMode::HSL_SATURATION));
    mode()->addItem(new BlendModeItem(Strings::layer_properties_color(),
                                      doc::BlendMode::HSL_COLOR));
    mode()->addItem(new BlendModeItem(Strings::layer_properties_luminosity(),
                                      doc::BlendMode::HSL_LUMINOSITY));

    name()->Change.connect([this]{ onStartTimer(); });
    mode()->Change.connect([this]{ onStartTimer(); });
    opacity()->Change.connect([this]{ onStartTimer(); });
    m_timer.Tick.connect([this]{ onCommitChange(); });
    userData()->Click.connect([this]{ onToggleUserData(); });
    tileset()->Click.connect([this]{ onTileset(); });
    tileset()->setVisible(false);

    m_userDataView.UserDataChange.connect([this]{ onStartTimer(); });

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

    if (countLayers() > 0) {
      m_userDataView.configureAndSet(m_layer->userData(),
                                     g_window->propertiesGrid());
    }

    updateFromLayer();
  }

private:

  std::string nameValue() const {
    return name()->text();
  }

  doc::BlendMode blendModeValue() const {
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
    m_pendingChanges = true;
  }

  void onCommitChange() {
    // Nothing to change
    if (!m_pendingChanges)
      return;

    // Nothing to do here, as there is no layer selected.
    if (!m_layer)
      return;

    base::ScopedValue switchSelf(m_selfUpdate, true);

    m_timer.stop();

    const int count = countLayers();

    std::string newName = nameValue();
    int newOpacity = opacityValue();
    const doc::UserData newUserData = m_userDataView.userData();
    doc::BlendMode newBlendMode = blendModeValue();

    if ((count > 1) ||
        (count == 1 && m_layer && (newName != m_layer->name() ||
                                   newUserData != m_layer->userData() ||
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
        const bool userDataChanged = (newUserData != m_layer->userData());
        const bool opacityChanged = (m_layer->isImage() && newOpacity != static_cast<LayerImage*>(m_layer)->opacity());
        const bool blendModeChanged = (m_layer->isImage() && newBlendMode != static_cast<LayerImage*>(m_layer)->blendMode());

        for (Layer* layer : range.selectedLayers()) {
          if (nameChanged && newName != layer->name())
            tx(new cmd::SetLayerName(layer, newName));

          if (userDataChanged && newUserData != layer->userData())
            tx(new cmd::SetUserData(layer, newUserData, m_document));

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
    onCommitChange();
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

  void onUserDataChange(DocEvent& ev) override {
    if (m_layer == ev.withUserData())
      updateFromLayer();
  }

  void onToggleUserData() {
    if (m_layer) {
      m_userDataView.toggleVisibility();
      g_window->remapWindow();
      manager()->invalidate();
    }
  }

  void onTileset() {
    if (!m_layer || !m_layer->isTilemap())
      return;

    auto tilemap = static_cast<LayerTilemap*>(m_layer);
    auto tileset = tilemap->tileset();

    // Information about the tileset to be used for new tilemaps
    TilesetSelector::Info tilesetInfo;
    tilesetInfo.enabled = false;
    tilesetInfo.newTileset = false;
    tilesetInfo.grid = tileset->grid();
    tilesetInfo.name = tileset->name();
    tilesetInfo.baseIndex = tileset->baseIndex();
    tilesetInfo.tsi = tilemap->tilesetIndex();

    try {
      gen::TilesetSelectorWindow window;
      TilesetSelector tilesetSel(tilemap->sprite(), tilesetInfo);
      window.tilesetOptions()->addChild(&tilesetSel);
      window.openWindowInForeground();
      if (window.closer() != window.ok())
        return;

      tilesetInfo = tilesetSel.getInfo();

      if (tileset->name() != tilesetInfo.name ||
          tileset->baseIndex() != tilesetInfo.baseIndex) {
        ContextWriter writer(UIContext::instance());
        Tx tx(writer.context(), "Set Tileset Properties");
        if (tileset->name() != tilesetInfo.name)
          tx(new cmd::SetTilesetName(tileset, tilesetInfo.name));
        if (tileset->baseIndex() != tilesetInfo.baseIndex)
          tx(new cmd::SetTilesetBaseIndex(tileset, tilesetInfo.baseIndex));
        // TODO catch the tileset base index modification from the editor
        App::instance()->mainWindow()->invalidate();
        tx.commit();
      }
    }
    catch (const std::exception& e) {
      Console::showException(e);
    }
  }

  void updateFromLayer() {
    if (m_selfUpdate)
      return;

    m_timer.stop(); // Cancel current editions (just in case)

    base::ScopedValue switchSelf(m_selfUpdate, true);

    const bool tilemapVisibility = (m_layer && m_layer->isTilemap());
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

      color_t c = m_layer->userData().color();
      m_userDataView.color()->setColor(Color::fromRgb(rgba_getr(c), rgba_getg(c), rgba_getb(c), rgba_geta(c)));
      m_userDataView.entry()->setText(m_layer->userData().text());
    }
    else {
      name()->setText(Strings::layer_properties_no_layer());
      name()->setEnabled(false);
      mode()->setEnabled(false);
      opacity()->setEnabled(false);
      m_userDataView.setVisible(false, false);
    }

    if (tileset()->isVisible() != tilemapVisibility) {
      tileset()->setVisible(tilemapVisibility);
      tileset()->parent()->layout();
    }
  }

  Timer m_timer;
  bool m_pendingChanges = false;
  Doc* m_document = nullptr;
  Layer* m_layer = nullptr;
  DocRange m_range;
  bool m_selfUpdate = false;
  UserDataView m_userDataView;
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
