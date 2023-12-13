// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/assign_color_profile.h"
#include "app/cmd/convert_color_profile.h"
#include "app/cmd/add_tileset.h"
#include "app/cmd/remove_tileset.h"
#include "app/cmd/set_pixel_ratio.h"
#include "app/cmd/set_user_data.h"
#include "app/color.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/doc_api.h"
#include "app/i18n/strings.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/util/tileset_utils.h"
#include "app/tx.h"
#include "app/ui/color_button.h"
#include "app/ui/user_data_view.h"
#include "app/ui/skin/skin_theme.h"
#include "app/util/pixel_ratio.h"
#include "base/mem_utils.h"
#include "doc/image.h"
#include "doc/palette.h"
#include "doc/sprite.h"
#include "doc/user_data.h"
#include "doc/tilesets.h"
#include "fmt/format.h"
#include "os/color_space.h"
#include "os/system.h"
#include "ui/ui.h"

#include "sprite_properties.xml.h"

namespace app {

using namespace ui;

class TilesetListItem : public ui::ListItem {
public:
  TilesetListItem(const doc::Tileset* tileset, doc::tileset_index tsi)
    : ListItem(app::tileset_label(tileset, tsi))
  {
    m_buttons.setTransparent(true);
    m_buttons.setExpansive(true);
    m_buttons.setVisible(false);

    auto filler = new BoxFiller();
    filler->setTransparent(true);
    m_buttons.addChild(filler);

    auto theme = skin::SkinTheme::get(this);
    auto duplicateBtn = new Button(Strings::sprite_properties_duplicate_tileset());
    duplicateBtn->setStyle(theme->styles.miniButton());
    duplicateBtn->setTransparent(true);
    duplicateBtn->Click.connect([this, tileset]{ onDuplicate(tileset); });
    m_buttons.addChild(duplicateBtn);

    auto deleteBtn = new Button(Strings::sprite_properties_delete_tileset());
    deleteBtn->setStyle(theme->styles.miniButton());
    deleteBtn->setTransparent(true);
    deleteBtn->Click.connect([this, tileset]{ onDelete(tileset); });
    m_buttons.addChild(deleteBtn);

    addChild(&m_buttons);
  }

  bool onProcessMessage(Message* msg) override
  {
    switch (msg->type()) {
      case kMouseLeaveMessage:
        m_buttons.setVisible(false);
        invalidate();
        break;
      case kMouseEnterMessage:
        m_buttons.setVisible(true);
        invalidate();
        break;
    }

    return ui::ListItem::onProcessMessage(msg);
  }

  obs::signal<void(TilesetListItem *)> TilesetDeleted;
  obs::signal<void(const Tileset*)> TilesetDuplicated;

private:
   void onDuplicate(const doc::Tileset* tileset)
   {
     auto sprite = tileset->sprite();
     auto tilesetClone = Tileset::MakeCopyCopyingImages(tileset);

     Tx tx(sprite, fmt::format(Strings::commands_TilesetDuplicate()));
     tx(new cmd::AddTileset(sprite, tilesetClone));
     tx.commit();

     TilesetDuplicated(tilesetClone);
   }

   void onDelete(const doc::Tileset* tileset)
   {
    doc::tileset_index tsi = tileset->sprite()->tilesets()->getIndex(tileset);
    std::string tilemapsNames;
    for (auto layer : tileset->sprite()->allTilemaps()) {
      auto tilemap = static_cast<doc::LayerTilemap*>(layer);
      if (tilemap->tilesetIndex() == tsi) {
        tilemapsNames += tilemap->name() + ", ";
      }
    }
    if (!tilemapsNames.empty()) {
      tilemapsNames = tilemapsNames.substr(0, tilemapsNames.size()-2);
      ui::Alert::show(fmt::format(Strings::alerts_cannot_delete_used_tileset(), tilemapsNames));
      return;
    }

    auto sprite = tileset->sprite();
    Tx tx(sprite, fmt::format(Strings::commands_TilesetDelete()));
    tx(new cmd::RemoveTileset(sprite, tsi));
    tx.commit();

    TilesetDeleted(this);
   }

  ui::HBox m_buttons;
};

class SpritePropertiesWindow : public app::gen::SpriteProperties {
public:
  SpritePropertiesWindow(Sprite* sprite)
    : SpriteProperties()
    , m_sprite(sprite)
    , m_userDataView(Preferences::instance().sprite.userDataVisibility)
  {
    userData()->Click.connect([this]{ onToggleUserData(); });

    m_userDataView.configureAndSet(m_sprite->userData(),
                                   propertiesGrid());

    if (sprite->tilesets()->size() == 0) {
      tilesetsPlaceholder()->parent()->removeChild(tilesetsPlaceholder());
    }
    else {
      for (int i = 0; i < sprite->tilesets()->size(); ++i) {
        auto tileset = (*sprite->tilesets()).get(i);
        addTilesetListItem(tileset, i);
      }

      Open.connect([this]{ adjustSize(); });
    }
  }

  const UserData& getUserData() const { return m_userDataView.userData(); }

protected:
  virtual void onSizeHint(SizeHintEvent& ev) override {
    app::gen::SpriteProperties::onSizeHint(ev);
    auto sz = ev.sizeHint();
    sz.h += getTilesetsViewHeight();
    ev.setSizeHint(sz);
  }

private:
  int getTilesetsViewHeight() {
    auto sz = tilesetsView()->viewport()->calculateNeededSize();
    return std::min(72, sz.h);
  }

  void addTilesetListItem(const doc::Tileset* tileset, doc::tileset_index tsi) {
    auto item = new TilesetListItem(tileset, tsi);
    item->TilesetDeleted.connect(&SpritePropertiesWindow::onTilesetDeleted, this);
    item->TilesetDuplicated.connect(&SpritePropertiesWindow::onTilesedDuplicated, this);
    tilesets()->addChild(item);
  }

  void onToggleUserData() {
    m_userDataView.toggleVisibility();
    remapWindow();
    manager()->invalidate();
  }

  void onTilesedDuplicated(const Tileset* tilesetClone) {
    addTilesetListItem(tilesetClone, tilesets()->children().size());
    layout();
  }

  void onTilesetDeleted(TilesetListItem *item) {
    int i = tilesets()->getChildIndex(item);
    tilesets()->removeChild(item);
    // Update text for items below the removed one, because tileset indexes
    // have changed.
    for (;i < tilesets()->children().size(); ++i) {
      auto it = tilesets()->children()[i];
      it->setText(app::tileset_label(m_sprite->tilesets()->get(i), i));
    }
    layout();
  }

  void adjustSize() {
    // If the tilesets view is too small, lets inflate the windows height a bit.
    if (tilesetsView()->childrenBounds().h < 36) {
      auto bounds = this->bounds();
      bounds.inflate(0, getTilesetsViewHeight() - tilesetsView()->clientBounds().h);
      setBounds(bounds);
      remapWindow();
    }
  }

  Sprite* m_sprite;
  UserDataView m_userDataView;
};

class SpritePropertiesCommand : public Command {
public:
  SpritePropertiesCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

SpritePropertiesCommand::SpritePropertiesCommand()
  : Command(CommandId::SpriteProperties(), CmdUIOnlyFlag)
{
}

bool SpritePropertiesCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void SpritePropertiesCommand::onExecute(Context* context)
{
  std::string imgtype_text;
  ColorButton* color_button = nullptr;

  // List of available color profiles
  std::vector<os::ColorSpaceRef> colorSpaces;
  os::instance()->listColorSpaces(colorSpaces);

  // Load the window widget
  SpritePropertiesWindow window(context->activeDocument()->sprite());

  int selectedColorProfile = -1;

  auto updateButtons =
    [&] {
      bool enabled = (selectedColorProfile != window.colorProfile()->getSelectedItemIndex());
      window.assignColorProfile()->setEnabled(enabled);
      window.convertColorProfile()->setEnabled(enabled);
      window.ok()->setEnabled(!enabled);
    };

  // Get sprite properties and fill frame fields
  {
    const ContextReader reader(context);
    const Doc* document(reader.document());
    const Sprite* sprite(reader.sprite());

    // Update widgets values
    switch (sprite->pixelFormat()) {
      case IMAGE_RGB:
        imgtype_text = Strings::sprite_properties_rgb();
        break;
      case IMAGE_GRAYSCALE:
        imgtype_text = Strings::sprite_properties_grayscale();
        break;
      case IMAGE_INDEXED:
        imgtype_text = fmt::format(Strings::sprite_properties_indexed_color(),
                                   sprite->palette(0)->size());
        break;
      default:
        ASSERT(false);
        imgtype_text = Strings::general_unknown();
        break;
    }

    // Filename
    window.name()->setText(document->filename());

    // Color mode
    window.type()->setText(imgtype_text.c_str());

    // Sprite size (width and height)
    window.size()->setTextf(
      "%dx%d (%s)",
      sprite->width(),
      sprite->height(),
      base::get_pretty_memory_size(sprite->getMemSize()).c_str());

    // How many frames
    window.frames()->setTextf("%d", (int)sprite->totalFrames());

    if (sprite->pixelFormat() == IMAGE_INDEXED) {
      color_button = new ColorButton(app::Color::fromIndex(sprite->transparentColor()),
                                     IMAGE_INDEXED,
                                     ColorButtonOptions());

      window.transparentColorPlaceholder()->addChild(color_button);

      // TODO add a way to get or create an existent TooltipManager
      TooltipManager* tooltipManager = new TooltipManager;
      window.addChild(tooltipManager);
      tooltipManager->addTooltipFor(
        color_button,
        Strings::sprite_properties_transparent_color_tooltip(),
        LEFT);
    }
    else {
      window.transparentColorPlaceholder()->addChild(
        new Label(Strings::sprite_properties_indexed_image_only()));
    }

    // Pixel ratio
    window.pixelRatio()->setValue(
      base::convert_to<std::string>(sprite->pixelRatio()));

    // Color profile
    selectedColorProfile = -1;
    int i = 0;
    for (auto& cs : colorSpaces) {
      if (cs->gfxColorSpace()->nearlyEqual(*sprite->colorSpace())) {
        selectedColorProfile = i;
        break;
      }
      ++i;
    }
    if (selectedColorProfile < 0) {
      colorSpaces.push_back(os::instance()->makeColorSpace(sprite->colorSpace()));
      selectedColorProfile = colorSpaces.size()-1;
    }

    for (auto& cs : colorSpaces)
      window.colorProfile()->addItem(cs->gfxColorSpace()->name());
    window.colorProfile()->setSelectedItemIndex(selectedColorProfile);

    window.assignColorProfile()->setEnabled(false);
    window.convertColorProfile()->setEnabled(false);
    window.colorProfile()->Change.connect(updateButtons);

    window.assignColorProfile()->Click.connect(
      [&](){
        selectedColorProfile = window.colorProfile()->getSelectedItemIndex();

        ContextWriter writer(context);
        Sprite* sprite(writer.sprite());
        Tx tx(writer, Strings::sprite_properties_assign_color_profile());
        tx(new cmd::AssignColorProfile(
             sprite, colorSpaces[selectedColorProfile]->gfxColorSpace()));
        tx.commit();

        updateButtons();
      });
    window.convertColorProfile()->Click.connect(
      [&](){
        selectedColorProfile = window.colorProfile()->getSelectedItemIndex();

        ContextWriter writer(context);
        Sprite* sprite(writer.sprite());
        Tx tx(writer, Strings::sprite_properties_convert_color_profile());
        tx(new cmd::ConvertColorProfile(
             sprite, colorSpaces[selectedColorProfile]->gfxColorSpace()));
        tx.commit();

        updateButtons();
      });
  }

  window.remapWindow();
  window.centerWindow();

  load_window_pos(&window, "SpriteProperties");
  window.setVisible(true);
  window.openWindowInForeground();

  if (window.closer() == window.ok()) {
    ContextWriter writer(context);
    Sprite* sprite(writer.sprite());

    color_t index = (color_button ? color_button->getColor().getIndex():
                                    sprite->transparentColor());
    PixelRatio pixelRatio =
      base::convert_to<PixelRatio>(window.pixelRatio()->getValue());

    const UserData newUserData = window.getUserData();

    if (index != sprite->transparentColor() ||
        pixelRatio != sprite->pixelRatio() ||
        newUserData != sprite->userData()) {
      Tx tx(writer, Strings::sprite_properties_change_sprite_props());
      DocApi api = writer.document()->getApi(tx);

      if (index != sprite->transparentColor())
        api.setSpriteTransparentColor(sprite, index);

      if (pixelRatio != sprite->pixelRatio())
        tx(new cmd::SetPixelRatio(sprite, pixelRatio));

      if (newUserData != sprite->userData())
        tx(new cmd::SetUserData(sprite, newUserData, static_cast<Doc*>(sprite->document())));

      tx.commit();

      update_screen_for_document(writer.document());
    }
  }

  save_window_pos(&window, "SpriteProperties");
}

Command* CommandFactory::createSpritePropertiesCommand()
{
  return new SpritePropertiesCommand;
}

} // namespace app
