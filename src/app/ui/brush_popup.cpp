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
#include "app/ui/button_set.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/skin/skin_theme.h"
#include "base/convert_to.h"
#include "doc/brush.h"
#include "doc/conversion_she.h"
#include "doc/image.h"
#include "doc/palette.h"
#include "gfx/border.h"
#include "gfx/region.h"
#include "she/surface.h"
#include "she/system.h"
#include "ui/tooltips.h"

namespace app {

using namespace app::skin;
using namespace doc;
using namespace ui;

class Item : public ButtonSet::Item {
public:
  Item(const BrushRef& brush)
    : m_brush(brush) {
    setIcon(BrushPopup::createSurfaceForBrush(brush));
  }

  ~Item() {
    icon()->dispose();
  }

  const BrushRef& brush() const { return m_brush; }

private:
  BrushRef m_brush;
};

static BrushRef defBrushes[3];

BrushPopup::BrushPopup()
  : PopupWindow("", kCloseOnClickInOtherWindow)
{
  setAutoRemap(false);
  setBorder(gfx::Border(0));
  child_spacing = 0;
}

void BrushPopup::setBrush(Brush* brush)
{
  for (auto child : m_buttons->getChildren()) {
    Item* item = static_cast<Item*>(child);

    // Same type and same image
    if (item->brush()->type() == brush->type() &&
        (brush->type() != kImageBrushType ||
         item->brush()->image() == brush->image())) {
      m_buttons->setSelectedItem(item);
      break;
    }
  }
}

void BrushPopup::regenerate(const gfx::Rect& box, const Brushes& brushes)
{
  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());

  if (m_buttons) {
    for (auto child : m_buttons->getChildren())
      m_tooltipManager->removeTooltipFor(child);
    removeChild(m_buttons.get());
    m_buttons.reset();
  }

  if (!defBrushes[0]) {
    defBrushes[0].reset(new Brush(kCircleBrushType, 7, 0));
    defBrushes[1].reset(new Brush(kSquareBrushType, 7, 0));
    defBrushes[2].reset(new Brush(kLineBrushType, 7, 44));
  }

  m_buttons.reset(new ButtonSet(3 + brushes.size()));
  m_buttons->addItem(new Item(defBrushes[0]));
  m_buttons->addItem(new Item(defBrushes[1]));
  m_buttons->addItem(new Item(defBrushes[2]));

  int slot = 1;
  for (const auto& brush : brushes) {
    Item* item = new Item(brush);
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
      m_tooltipManager->addTooltipFor(item, tooltip, JI_TOP);
    }
    slot++;
  }

  m_buttons->ItemChange.connect(&BrushPopup::onButtonChange, this);
  m_buttons->setTransparent(true);
  m_buttons->setBgColor(gfx::ColorNone);
  addChild(m_buttons.get());

  gfx::Rect rc = box;
  rc.w *= m_buttons->getChildren().size();
  setBounds(rc);
}

void BrushPopup::onButtonChange()
{
  Item* item = static_cast<Item*>(m_buttons->getItem(m_buttons->selectedItem()));
  BrushChange(item->brush());
}

// static
she::Surface* BrushPopup::createSurfaceForBrush(const BrushRef& origBrush)
{
  BrushRef brush = origBrush;

  if (brush->type() != kImageBrushType && brush->size() > 10) {
    brush.reset(new Brush(*brush));
    brush->setSize(10);
  }

  Image* image = brush->image();

  she::Surface* surface = she::instance()->createRgbaSurface(
    std::min(10, image->width()),
    std::min(10, image->height()));

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

  return surface;
}

} // namespace app
