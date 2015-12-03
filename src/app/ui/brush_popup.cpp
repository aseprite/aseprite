// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/brush_popup.h"

#include "app/commands/commands.h"
#include "app/modules/palettes.h"
#include "app/ui/app_menuitem.h"
#include "app/ui/button_set.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/skin/skin_theme.h"
#include "base/bind.h"
#include "base/convert_to.h"
#include "doc/brush.h"
#include "doc/conversion_she.h"
#include "doc/image.h"
#include "doc/palette.h"
#include "gfx/border.h"
#include "gfx/region.h"
#include "she/scoped_surface_lock.h"
#include "she/surface.h"
#include "she/system.h"
#include "ui/menu.h"
#include "ui/message.h"
#include "ui/separator.h"
#include "ui/tooltips.h"

namespace app {

using namespace app::skin;
using namespace doc;
using namespace ui;

namespace {

class Item : public ButtonSet::Item {
public:
  Item(BrushPopup* popup, BrushPopupDelegate* delegate, const BrushRef& brush, int slot = -1)
    : m_popup(popup)
    , m_delegate(delegate)
    , m_brush(brush)
    , m_slot(slot) {
    SkinPartPtr icon(new SkinPart);
    icon->setBitmap(0, BrushPopup::createSurfaceForBrush(brush));
    setIcon(icon);
  }

  const BrushRef& brush() const {
    return m_brush;
  }

protected:
  bool onProcessMessage(Message* msg) override {
    if (msg->type() == kMouseUpMessage && m_slot > 0) {
      MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
      if (mouseMsg->buttons() == kButtonRight) {
        Menu menu;
        AppMenuItem lockItem(m_delegate->onIsBrushSlotLocked(m_slot) ? "Unlock Brush": "Lock Brush");
        AppMenuItem deleteItem("Delete");
        AppMenuItem deleteAllItem("Delete All");
        lockItem.Click.connect(&Item::onLockBrush, this);
        deleteItem.Click.connect(&Item::onDeleteBrush, this);
        deleteAllItem.Click.connect(&Item::onDeleteAllBrushes, this);
        menu.addChild(&lockItem);
        menu.addChild(&deleteItem);
        menu.addChild(new MenuSeparator);
        menu.addChild(&deleteAllItem);

        // Here we make the popup window temporaly floating, so it's
        // not closed by the popup menu.
        m_popup->makeFloating();
        menu.showPopup(mouseMsg->position());
        m_popup->makeFixed();
        m_popup->closeWindow(nullptr);
      }
    }
    return ButtonSet::Item::onProcessMessage(msg);
  }

private:
  void onLockBrush() {
    if (m_delegate->onIsBrushSlotLocked(m_slot))
      m_delegate->onUnlockBrushSlot(m_slot);
    else
      m_delegate->onLockBrushSlot(m_slot);
  }

  void onDeleteBrush() {
    m_delegate->onDeleteBrushSlot(m_slot);
  }

  void onDeleteAllBrushes() {
    m_delegate->onDeleteAllBrushes();
  }

  BrushPopup* m_popup;
  BrushPopupDelegate* m_delegate;
  BrushRef m_brush;
  int m_slot;
};

} // anonymous namespace

static BrushRef defBrushes[3];

BrushPopup::BrushPopup(BrushPopupDelegate* delegate)
  : PopupWindow("", kCloseOnClickInOtherWindow)
  , m_delegate(delegate)
{
  setAutoRemap(false);
  setBorder(gfx::Border(0));
  setChildSpacing(0);
}

void BrushPopup::setBrush(Brush* brush)
{
  for (auto child : m_buttons->children()) {
    Item* item = static_cast<Item*>(child);

    // Same type and same image
    if (item->brush() &&
        item->brush()->type() == brush->type() &&
        (brush->type() != kImageBrushType ||
         item->brush()->image() == brush->image())) {
      m_buttons->setSelectedItem(item);
      break;
    }
  }
}

void BrushPopup::regenerate(const gfx::Rect& box, const Brushes& brushes)
{
  int columns = 3;

  if (m_buttons) {
    for (auto child : m_buttons->children())
      m_tooltipManager->removeTooltipFor(child);
    removeChild(m_buttons.get());
    m_buttons.reset();
  }

  if (!defBrushes[0]) {
    defBrushes[0].reset(new Brush(kCircleBrushType, 7, 0));
    defBrushes[1].reset(new Brush(kSquareBrushType, 7, 0));
    defBrushes[2].reset(new Brush(kLineBrushType, 7, 44));
  }

  m_buttons.reset(new ButtonSet(columns));
  m_buttons->addItem(new Item(this, m_delegate, defBrushes[0]));
  m_buttons->addItem(new Item(this, m_delegate, defBrushes[1]));
  m_buttons->addItem(new Item(this, m_delegate, defBrushes[2]));

  int slot = 1;
  for (const auto& brush : brushes) {
    Item* item = new Item(this, m_delegate, brush, slot);
    m_buttons->addItem(item);

    Params params;
    params.set("change", "custom");
    params.set("slot", base::convert_to<std::string>(slot).c_str());
    Key* key = KeyboardShortcuts::instance()->command(
      CommandId::ChangeBrush, params);
    if (key && !key->accels().empty()) {
      std::string tooltip;
      tooltip += "Shortcut: ";
      tooltip += key->accels().front().toString();
      m_tooltipManager->addTooltipFor(item, tooltip, TOP);
    }
    slot++;
  }
  // Add empty spaces
  while (((slot-1) % columns) > 0)
    m_buttons->addItem(new Item(this, m_delegate, BrushRef(nullptr), slot++));

  m_buttons->ItemChange.connect(Bind<void>(&BrushPopup::onButtonChange, this));
  m_buttons->setTransparent(true);
  m_buttons->setBgColor(gfx::ColorNone);
  addChild(m_buttons.get());

  gfx::Rect rc = box;
  int buttons = m_buttons->children().size();
  int rows = (buttons/columns + ((buttons%columns) > 0 ? 1: 0));
  rc.w *= columns;
  rc.h = rows * (rc.h-2*guiscale()) + 2*guiscale();

  setBounds(rc);
}

void BrushPopup::onButtonChange()
{
  Item* item = static_cast<Item*>(m_buttons->getItem(m_buttons->selectedItem()));
  if (item->brush())
    BrushChange(item->brush());
}

// static
she::Surface* BrushPopup::createSurfaceForBrush(const BrushRef& origBrush)
{
  Image* image = nullptr;
  BrushRef brush = origBrush;
  if (brush) {
    if (brush->type() != kImageBrushType && brush->size() > 10) {
      brush.reset(new Brush(*brush));
      brush->setSize(10);
    }
    image = brush->image();
  }

  she::Surface* surface = she::instance()->createRgbaSurface(
    std::min(10, image ? image->width(): 4),
    std::min(10, image ? image->height(): 4));

  if (image) {
    Palette* palette = get_current_palette();
    if (image->pixelFormat() == IMAGE_BITMAP) {
      palette = new Palette(frame_t(0), 2);
      palette->setEntry(0, rgba(0, 0, 0, 0));
      palette->setEntry(1, rgba(0, 0, 0, 255));
    }

    convert_image_to_surface(
      image, palette, surface,
      0, 0, 0, 0, image->width(), image->height());

    if (image->pixelFormat() == IMAGE_BITMAP)
      delete palette;
  }
  else {
    she::ScopedSurfaceLock lock(surface);
    lock->clear();
  }

  return surface;
}

} // namespace app
