/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "config.h"

#include <allegro.h>
#include <vector>

#include "app.h"
#include "app/color.h"
#include "base/bind.h"
#include "gfx/border.h"
#include "gfx/size.h"
#include "gui/gui.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "widgets/color_selector.h"
#include "widgets/palette_view.h"

ColorSelector::ColorSelector()
  : PopupFrame("Color Selector", false)
  , m_color(Color::fromMask())
  , m_vbox(JI_VERTICAL)
  , m_topBox(JI_HORIZONTAL)
  , m_colorPalette(false)
  , m_indexButton("Index", 1, JI_BUTTON)
  , m_rgbButton("RGB", 1, JI_BUTTON)
  , m_hsvButton("HSV", 1, JI_BUTTON)
  , m_grayButton("Gray", 1, JI_BUTTON)
  , m_maskButton("Mask", 1, JI_BUTTON)
  , m_maskLabel("Transparent Color Selected")
  , m_disableHexUpdate(false)
{
  m_topBox.setBorder(gfx::Border(0));
  m_topBox.child_spacing = 0;

  m_colorPalette.setColumns(40);
  m_colorPalette.setBoxSize(6*jguiscale());
  m_colorPaletteContainer.attachToView(&m_colorPalette);
 
  jwidget_expansive(&m_colorPaletteContainer, true);

  setup_mini_look(&m_indexButton);
  setup_mini_look(&m_rgbButton);
  setup_mini_look(&m_hsvButton);
  setup_mini_look(&m_grayButton);
  setup_mini_look(&m_maskButton);

  jwidget_add_child(&m_topBox, &m_indexButton);
  jwidget_add_child(&m_topBox, &m_rgbButton);
  jwidget_add_child(&m_topBox, &m_hsvButton);
  jwidget_add_child(&m_topBox, &m_grayButton);
  jwidget_add_child(&m_topBox, &m_maskButton);
  jwidget_add_child(&m_topBox, &m_hexColorEntry);
  jwidget_add_child(&m_vbox, &m_topBox);
  jwidget_add_child(&m_vbox, &m_colorPaletteContainer);
  jwidget_add_child(&m_vbox, &m_rgbSliders);
  jwidget_add_child(&m_vbox, &m_hsvSliders);
  jwidget_add_child(&m_vbox, &m_graySlider);
  jwidget_add_child(&m_vbox, &m_maskLabel);
  jwidget_add_child(this, &m_vbox);

  m_indexButton.Click.connect(&ColorSelector::onColorTypeButtonClick, this);
  m_rgbButton.Click.connect(&ColorSelector::onColorTypeButtonClick, this);
  m_hsvButton.Click.connect(&ColorSelector::onColorTypeButtonClick, this);
  m_grayButton.Click.connect(&ColorSelector::onColorTypeButtonClick, this);
  m_maskButton.Click.connect(&ColorSelector::onColorTypeButtonClick, this);

  m_colorPalette.IndexChange.connect(&ColorSelector::onColorPaletteIndexChange, this);
  m_rgbSliders.ColorChange.connect(&ColorSelector::onColorSlidersChange, this);
  m_hsvSliders.ColorChange.connect(&ColorSelector::onColorSlidersChange, this);
  m_graySlider.ColorChange.connect(&ColorSelector::onColorSlidersChange, this);
  m_hexColorEntry.ColorChange.connect(&ColorSelector::onColorHexEntryChange, this);

  selectColorType(Color::RgbType);
  setPreferredSize(gfx::Size(300*jguiscale(), getPreferredSize().h));

  initTheme();
}

void ColorSelector::setColor(const Color& color)
{
  m_color = color;

  if (color.getType() == Color::IndexType)
    m_colorPalette.selectColor(color.getIndex());

  m_rgbSliders.setColor(m_color);
  m_hsvSliders.setColor(m_color);
  m_graySlider.setColor(m_color);
  if (!m_disableHexUpdate)
    m_hexColorEntry.setColor(m_color);

  selectColorType(m_color.getType());
}

Color ColorSelector::getColor() const
{
  return m_color;
}

void ColorSelector::onColorPaletteIndexChange(int index)
{
  setColorWithSignal(Color::fromIndex(index));
}

void ColorSelector::onColorSlidersChange(ColorSlidersChangeEvent& ev)
{
  setColorWithSignal(ev.getColor());
  findBestfitIndex(ev.getColor());
}

void ColorSelector::onColorHexEntryChange(const Color& color)
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

  if (source == &m_indexButton) selectColorType(Color::IndexType);
  else if (source == &m_rgbButton) selectColorType(Color::RgbType);
  else if (source == &m_hsvButton) selectColorType(Color::HsvType);
  else if (source == &m_grayButton) selectColorType(Color::GrayType);
  else if (source == &m_maskButton) {
    // Select mask color directly when the radio button is pressed
    setColorWithSignal(Color::fromMask());
  }
}

void ColorSelector::findBestfitIndex(const Color& color)
{
  // Find bestfit palette entry
  int r = color.getRed();
  int g = color.getGreen();
  int b = color.getBlue();
  
  // Search for the closest color to the RGB values
  int i = get_current_palette()->findBestfit(r, g, b);
  if (i >= 0 && i < 256)
    m_colorPalette.selectColor(i);
}

void ColorSelector::setColorWithSignal(const Color& color)
{
  setColor(color);

  // Fire ColorChange signal
  ColorChange(color);
}

void ColorSelector::selectColorType(Color::Type type)
{
  m_colorPaletteContainer.setVisible(type == Color::IndexType);
  m_rgbSliders.setVisible(type == Color::RgbType);
  m_hsvSliders.setVisible(type == Color::HsvType);
  m_graySlider.setVisible(type == Color::GrayType);
  m_maskLabel.setVisible(type == Color::MaskType);

  switch (type) {
    case Color::IndexType: m_indexButton.setSelected(true); break;
    case Color::RgbType:   m_rgbButton.setSelected(true); break;
    case Color::HsvType:   m_hsvButton.setSelected(true); break;
    case Color::GrayType:  m_grayButton.setSelected(true); break;
    case Color::MaskType:  m_maskButton.setSelected(true); break;
  }
  
  m_vbox.layout();
  m_vbox.invalidate();
}
