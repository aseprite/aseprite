/* Aseprite
 * Copyright (C) 2001-2014  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <allegro.h>
#include <vector>

#include "app/app.h"
#include "app/color.h"
#include "app/console.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/document.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/ui/color_selector.h"
#include "app/ui/palette_view.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/skin/style.h"
#include "app/ui_context.h"
#include "app/undo_transaction.h"
#include "app/undoers/set_palette_colors.h"
#include "base/bind.h"
#include "base/scoped_value.h"
#include "gfx/border.h"
#include "gfx/size.h"
#include "raster/image.h"
#include "raster/image_bits.h"
#include "raster/palette.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "ui/ui.h"

namespace app {

using namespace ui;
using namespace raster;

class ColorSelector::WarningIcon : public Button {
public:
  WarningIcon() : Button("") {
  }

private:
  skin::Style* style() {
    return skin::get_style("warning_box");
  }

  bool onProcessMessage(Message* msg) OVERRIDE {
    switch (msg->type()) {
      case kSetCursorMessage:
        jmouse_set_cursor(kHandCursor);
        return true;
    }
    return Button::onProcessMessage(msg);
  }

  void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE {
    ev.setPreferredSize(
      style()->preferredSize(NULL, skin::Style::State()) + 4*jguiscale());
  }

  void onPaint(PaintEvent& ev) OVERRIDE {
    Graphics* g = ev.getGraphics();
    skin::Style::State state;
    if (hasMouse()) state += skin::Style::hover();
    if (isSelected()) state += skin::Style::clicked();
    style()->paint(g, getClientBounds(), NULL, state);
  }
};

ColorSelector::ColorSelector()
  : PopupWindowPin("Color Selector", PopupWindow::kCloseOnClickInOtherWindow)
  , m_color(app::Color::fromMask())
  , m_vbox(JI_VERTICAL)
  , m_topBox(JI_HORIZONTAL)
  , m_colorPalette(false)
  , m_indexButton("Index", 1, kButtonWidget)
  , m_rgbButton("RGB", 1, kButtonWidget)
  , m_hsvButton("HSB", 1, kButtonWidget)
  , m_grayButton("Gray", 1, kButtonWidget)
  , m_maskButton("Mask", 1, kButtonWidget)
  , m_maskLabel("Transparent Color Selected")
  , m_warningIcon(new WarningIcon)
  , m_disableHexUpdate(false)
{
  m_topBox.setBorder(gfx::Border(0));
  m_topBox.child_spacing = 0;

  m_colorPalette.setBoxSize(6*jguiscale());
  m_colorPaletteContainer.attachToView(&m_colorPalette);

  m_colorPaletteContainer.setExpansive(true);

  setup_mini_look(&m_indexButton);
  setup_mini_look(&m_rgbButton);
  setup_mini_look(&m_hsvButton);
  setup_mini_look(&m_grayButton);
  setup_mini_look(&m_maskButton);

  m_topBox.addChild(&m_indexButton);
  m_topBox.addChild(&m_rgbButton);
  m_topBox.addChild(&m_hsvButton);
  m_topBox.addChild(&m_grayButton);
  m_topBox.addChild(&m_maskButton);
  m_topBox.addChild(&m_hexColorEntry);
  m_topBox.addChild(m_warningIcon);
  {
    Box* miniVbox = new Box(JI_VERTICAL);
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

  m_indexButton.Click.connect(&ColorSelector::onColorTypeButtonClick, this);
  m_rgbButton.Click.connect(&ColorSelector::onColorTypeButtonClick, this);
  m_hsvButton.Click.connect(&ColorSelector::onColorTypeButtonClick, this);
  m_grayButton.Click.connect(&ColorSelector::onColorTypeButtonClick, this);
  m_maskButton.Click.connect(&ColorSelector::onColorTypeButtonClick, this);
  m_warningIcon->Click.connect(&ColorSelector::onFixWarningClick, this);

  m_colorPalette.IndexChange.connect(&ColorSelector::onColorPaletteIndexChange, this);
  m_rgbSliders.ColorChange.connect(&ColorSelector::onColorSlidersChange, this);
  m_hsvSliders.ColorChange.connect(&ColorSelector::onColorSlidersChange, this);
  m_graySlider.ColorChange.connect(&ColorSelector::onColorSlidersChange, this);
  m_hexColorEntry.ColorChange.connect(&ColorSelector::onColorHexEntryChange, this);

  selectColorType(app::Color::RgbType);
  setPreferredSize(gfx::Size(300*jguiscale(), getPreferredSize().h));

  m_onPaletteChangeConn =
    App::instance()->PaletteChange.connect(&ColorSelector::onPaletteChange, this);

  m_tooltips.addTooltipFor(m_warningIcon, "This color isn't in the palette\nPress here to add it.", JI_BOTTOM);

  initTheme();
}

ColorSelector::~ColorSelector()
{
  getPin()->getParent()->removeChild(getPin());
}

void ColorSelector::setColor(const app::Color& color, SetColorOptions options)
{
  m_color = color;

  if (color.getType() == app::Color::IndexType) {
    m_colorPalette.clearSelection();
    m_colorPalette.selectColor(color.getIndex());
  }

  m_rgbSliders.setColor(m_color);
  m_hsvSliders.setColor(m_color);
  m_graySlider.setColor(m_color);
  if (!m_disableHexUpdate)
    m_hexColorEntry.setColor(m_color);

  if (options == ChangeType)
    selectColorType(m_color.getType());

  int index = get_current_palette()->findExactMatch(
    m_color.getRed(),
    m_color.getGreen(),
    m_color.getBlue());

  m_warningIcon->setVisible(index < 0);
  m_warningIcon->getParent()->layout();
}

app::Color ColorSelector::getColor() const
{
  return m_color;
}

void ColorSelector::onColorPaletteIndexChange(int index)
{
  setColorWithSignal(app::Color::fromIndex(index));
}

void ColorSelector::onColorSlidersChange(ColorSlidersChangeEvent& ev)
{
  setColorWithSignal(ev.getColor());
  findBestfitIndex(ev.getColor());
}

void ColorSelector::onColorHexEntryChange(const app::Color& color)
{
  // Disable updating the hex entry so we don't override what the user
  // is writting in the text field.
  m_disableHexUpdate = true;

  setColorWithSignal(color);
  findBestfitIndex(color);

  m_disableHexUpdate = false;
}

void ColorSelector::onColorTypeButtonClick(Event& ev)
{
  RadioButton* source = static_cast<RadioButton*>(ev.getSource());
  app::Color newColor;

  if (source == &m_indexButton) newColor = app::Color::fromIndex(getColor().getIndex());
  else if (source == &m_rgbButton) newColor = app::Color::fromRgb(getColor().getRed(), getColor().getGreen(), getColor().getBlue());
  else if (source == &m_hsvButton) newColor = app::Color::fromHsv(getColor().getHue(), getColor().getSaturation(), getColor().getValue());
  else if (source == &m_grayButton) newColor = app::Color::fromGray(getColor().getGray());
  else if (source == &m_maskButton) newColor = app::Color::fromMask();

  setColorWithSignal(newColor);
}

void ColorSelector::onFixWarningClick(ui::Event& ev)
{
  try {
    Palette* newPalette = get_current_palette(); // System current pal
    color_t newColor = rgba(
      m_color.getRed(),
      m_color.getGreen(),
      m_color.getBlue());
    int index = newPalette->findExactMatch(
      m_color.getRed(),
      m_color.getGreen(),
      m_color.getBlue());

    // It should be -1, because the user has pressed the warning
    // button that is available only when the color isn't in the
    // palette.
    ASSERT(index < 0);
    if (index >= 0)
      return;

    int lastUsed = -1;

    ContextWriter writer(UIContext::instance());
    Document* document(writer.document());
    Sprite* sprite = NULL;
    if (document) {
      sprite = writer.sprite();

      // Find used entries in all stock images. In this way we can start
      // looking for duplicated color entries in the palette from the
      // last used one.
      if (sprite->getPixelFormat() == IMAGE_INDEXED) {
        lastUsed = sprite->getTransparentColor();

        Stock* stock = sprite->getStock();
        for (int i=0; i<(int)stock->size(); ++i) {
          Image* image = stock->getImage(i);
          if (!image)
            continue;

          const LockImageBits<IndexedTraits> bits(image);
          for (LockImageBits<IndexedTraits>::const_iterator it=bits.begin(); it!=bits.end(); ++it) {
            if (lastUsed < *it)
              lastUsed = *it;
          }
        }
      }
    }

    for (int i=lastUsed+1; i<(int)newPalette->size(); ++i) {
      color_t c = newPalette->getEntry(i);
      int altI = newPalette->findExactMatch(
        rgba_getr(c), rgba_getg(c), rgba_getb(c));
      if (altI < i) {
        index = i;
        break;
      }
    }

    if (index < 0) {
      if (newPalette->size() < Palette::MaxColors) {
        newPalette->addEntry(newColor);
        index = newPalette->size()-1;
      }

      if (index < 0) {
        Alert::show(
          "Error<<The palette is full."
          "<<You cannot have more than %d colors.<<&OK",
          Palette::MaxColors);
        return;
      }
    }
    else {
      newPalette->setEntry(index, newColor);
    }

    if (document) {
      FrameNumber frame = writer.frame();

      UndoTransaction undoTransaction(writer.context(), "Add palette entry", undo::ModifyDocument);
      undoTransaction.pushUndoer
        (new undoers::SetPaletteColors(undoTransaction.getObjects(),
          sprite, sprite->getPalette(frame),
          frame, index, index));

      sprite->setPalette(newPalette, false);

      undoTransaction.commit();
    }

    set_current_palette(newPalette, false);
    ui::Manager::getDefault()->invalidate();

    m_warningIcon->setVisible(index < 0);
    m_warningIcon->getParent()->layout();
  }
  catch (base::Exception& e) {
    Console::showException(e);
  }
}

void ColorSelector::onPaletteChange()
{
  setColor(getColor(), DoNotChangeType);
  invalidate();
}

void ColorSelector::findBestfitIndex(const app::Color& color)
{
  // Find bestfit palette entry
  int r = color.getRed();
  int g = color.getGreen();
  int b = color.getBlue();

  // Search for the closest color to the RGB values
  int i = get_current_palette()->findBestfit(r, g, b);
  if (i >= 0 && i < 256) {
    m_colorPalette.clearSelection();
    m_colorPalette.selectColor(i);
  }
}

void ColorSelector::setColorWithSignal(const app::Color& color)
{
  setColor(color, ChangeType);

  // Fire ColorChange signal
  ColorChange(color);
}

void ColorSelector::selectColorType(app::Color::Type type)
{
  m_colorPaletteContainer.setVisible(type == app::Color::IndexType);
  m_rgbSliders.setVisible(type == app::Color::RgbType);
  m_hsvSliders.setVisible(type == app::Color::HsvType);
  m_graySlider.setVisible(type == app::Color::GrayType);
  m_maskLabel.setVisible(type == app::Color::MaskType);

  switch (type) {
    case app::Color::IndexType: m_indexButton.setSelected(true); break;
    case app::Color::RgbType:   m_rgbButton.setSelected(true); break;
    case app::Color::HsvType:   m_hsvButton.setSelected(true); break;
    case app::Color::GrayType:  m_grayButton.setSelected(true); break;
    case app::Color::MaskType:  m_maskButton.setSelected(true); break;
  }

  m_vbox.layout();
  m_vbox.invalidate();
}

} // namespace app
