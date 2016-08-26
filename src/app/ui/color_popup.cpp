// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/color_popup.h"

#include "app/app.h"
#include "app/cmd/set_palette.h"
#include "app/color.h"
#include "app/console.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/document.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/transaction.h"
#include "app/ui/palette_view.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/skin/style.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/scoped_value.h"
#include "doc/image_impl.h"
#include "doc/palette.h"
#include "doc/sprite.h"
#include "gfx/border.h"
#include "gfx/size.h"
#include "ui/ui.h"

namespace app {

using namespace ui;
using namespace doc;

enum {
  INDEX_MODE,
  RGB_MODE,
  HSB_MODE,
  GRAY_MODE,
  MASK_MODE
};

ColorPopup::ColorPopup()
  : PopupWindowPin("Color Selector", ClickBehavior::CloseOnClickInOtherWindow)
  , m_vbox(VERTICAL)
  , m_topBox(HORIZONTAL)
  , m_color(app::Color::fromMask())
  , m_colorPalette(false, PaletteView::SelectOneColor, this, 7*guiscale())
  , m_colorType(5)
  , m_maskLabel("Transparent Color Selected")
  , m_disableHexUpdate(false)
{
  m_colorType.addItem("Index");
  m_colorType.addItem("RGB");
  m_colorType.addItem("HSB");
  m_colorType.addItem("Gray");
  m_colorType.addItem("Mask");

  m_topBox.setBorder(gfx::Border(0));
  m_topBox.setChildSpacing(0);

  m_colorPaletteContainer.attachToView(&m_colorPalette);

  m_colorPaletteContainer.setExpansive(true);

  m_topBox.addChild(&m_colorType);
  m_topBox.addChild(new Separator("", VERTICAL));
  m_topBox.addChild(&m_hexColorEntry);
  {
    Box* miniVbox = new Box(VERTICAL);
    miniVbox->addChild(getPin());
    m_topBox.addChild(new BoxFiller);
    m_topBox.addChild(miniVbox);
  }
  m_vbox.addChild(&m_topBox);
  m_vbox.addChild(&m_colorPaletteContainer);
  m_vbox.addChild(&m_rgbSliders);
  m_vbox.addChild(&m_hsvSliders);
  m_vbox.addChild(&m_graySlider);
  m_vbox.addChild(&m_maskLabel);
  addChild(&m_vbox);

  m_colorType.ItemChange.connect(base::Bind<void>(&ColorPopup::onColorTypeClick, this));

  m_rgbSliders.ColorChange.connect(&ColorPopup::onColorSlidersChange, this);
  m_hsvSliders.ColorChange.connect(&ColorPopup::onColorSlidersChange, this);
  m_graySlider.ColorChange.connect(&ColorPopup::onColorSlidersChange, this);
  m_hexColorEntry.ColorChange.connect(&ColorPopup::onColorHexEntryChange, this);

  selectColorType(app::Color::RgbType);
  setSizeHint(gfx::Size(300*guiscale(), sizeHint().h));

  m_onPaletteChangeConn =
    App::instance()->PaletteChange.connect(&ColorPopup::onPaletteChange, this);

  initTheme();
}

ColorPopup::~ColorPopup()
{
  getPin()->parent()->removeChild(getPin());
}

void ColorPopup::setColor(const app::Color& color, SetColorOptions options)
{
  m_color = color;

  if (color.getType() == app::Color::IndexType) {
    m_colorPalette.deselect();
    m_colorPalette.selectColor(color.getIndex());
  }

  m_rgbSliders.setColor(m_color);
  m_hsvSliders.setColor(m_color);
  m_graySlider.setColor(m_color);
  if (!m_disableHexUpdate)
    m_hexColorEntry.setColor(m_color);

  if (options == ChangeType)
    selectColorType(m_color.getType());
}

app::Color ColorPopup::getColor() const
{
  return m_color;
}

void ColorPopup::onPaletteViewIndexChange(int index, ui::MouseButtons buttons)
{
  setColorWithSignal(app::Color::fromIndex(index));
}

void ColorPopup::onColorSlidersChange(ColorSlidersChangeEvent& ev)
{
  setColorWithSignal(ev.color());
  findBestfitIndex(ev.color());
}

void ColorPopup::onColorHexEntryChange(const app::Color& color)
{
  // Disable updating the hex entry so we don't override what the user
  // is writting in the text field.
  m_disableHexUpdate = true;

  setColorWithSignal(color);
  findBestfitIndex(color);

  m_disableHexUpdate = false;
}

void ColorPopup::onColorTypeClick()
{
  app::Color newColor = getColor();

  switch (m_colorType.selectedItem()) {
    case INDEX_MODE:
      newColor = app::Color::fromIndex(newColor.getIndex());
      break;
    case RGB_MODE:
      newColor = app::Color::fromRgb(newColor.getRed(),
                                     newColor.getGreen(),
                                     newColor.getBlue(),
                                     newColor.getAlpha());
      break;
    case HSB_MODE:
      newColor = app::Color::fromHsv(newColor.getHue(),
                                     newColor.getSaturation(),
                                     newColor.getValue(),
                                     newColor.getAlpha());
      break;
    case GRAY_MODE:
      newColor = app::Color::fromGray(newColor.getGray(),
                                      newColor.getAlpha());
      break;
    case MASK_MODE:
      newColor = app::Color::fromMask();
      break;
  }

  setColorWithSignal(newColor);
}

void ColorPopup::onPaletteChange()
{
  setColor(getColor(), DoNotChangeType);
  invalidate();
}

void ColorPopup::findBestfitIndex(const app::Color& color)
{
  // Find bestfit palette entry
  int r = color.getRed();
  int g = color.getGreen();
  int b = color.getBlue();
  int a = color.getAlpha();

  // Search for the closest color to the RGB values
  int i = get_current_palette()->findBestfit(r, g, b, a, 0);
  if (i >= 0) {
    m_colorPalette.deselect();
    m_colorPalette.selectColor(i);
  }
}

void ColorPopup::setColorWithSignal(const app::Color& color)
{
  setColor(color, ChangeType);

  // Fire ColorChange signal
  ColorChange(color);
}

void ColorPopup::selectColorType(app::Color::Type type)
{
  m_colorPaletteContainer.setVisible(type == app::Color::IndexType);
  m_rgbSliders.setVisible(type == app::Color::RgbType);
  m_hsvSliders.setVisible(type == app::Color::HsvType);
  m_graySlider.setVisible(type == app::Color::GrayType);
  m_maskLabel.setVisible(type == app::Color::MaskType);

  switch (type) {
    case app::Color::IndexType: m_colorType.setSelectedItem(INDEX_MODE); break;
    case app::Color::RgbType:   m_colorType.setSelectedItem(RGB_MODE); break;
    case app::Color::HsvType:   m_colorType.setSelectedItem(HSB_MODE); break;
    case app::Color::GrayType:  m_colorType.setSelectedItem(GRAY_MODE); break;
    case app::Color::MaskType:  m_colorType.setSelectedItem(MASK_MODE); break;
  }

  m_vbox.layout();
  m_vbox.invalidate();
}

} // namespace app
