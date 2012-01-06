/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2012  David Capello
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

#ifndef WIDGETS_COLOR_SELECTOR_H_INCLUDED
#define WIDGETS_COLOR_SELECTOR_H_INCLUDED

#include "app/color.h"
#include "base/signal.h"
#include "gui/button.h"
#include "gui/grid.h"
#include "gui/label.h"
#include "gui/view.h"
#include "widgets/color_sliders.h"
#include "widgets/popup_frame_pin.h"
#include "widgets/hex_color_entry.h"
#include "widgets/palette_view.h"

class ColorSelector : public PopupFramePin
{
public:
  enum SetColorOptions {
    ChangeType,
    DoNotChangeType
  };

  ColorSelector();
  ~ColorSelector();

  void setColor(const Color& color, SetColorOptions options);
  Color getColor() const;

  // Signals
  Signal1<void, const Color&> ColorChange;

protected:
  void onColorPaletteIndexChange(int index);
  void onColorSlidersChange(ColorSlidersChangeEvent& ev);
  void onColorHexEntryChange(const Color& color);
  void onColorTypeButtonClick(Event& ev);
  void onPaletteChange();

private:
  void selectColorType(Color::Type type);
  void setColorWithSignal(const Color& color);
  void findBestfitIndex(const Color& color);

  Box m_vbox;
  Box m_topBox;
  Color m_color;
  View m_colorPaletteContainer;
  PaletteView m_colorPalette;
  RadioButton m_indexButton;
  RadioButton m_rgbButton;
  RadioButton m_hsvButton;
  RadioButton m_grayButton;
  RadioButton m_maskButton;
  HexColorEntry m_hexColorEntry;
  RgbSliders m_rgbSliders;
  HsvSliders m_hsvSliders;
  GraySlider m_graySlider;
  Label m_maskLabel;
  Signal0<void>::SlotType* m_onPaletteChangeSlot;

  // This variable is used to avoid updating the m_hexColorEntry text
  // when the color change is generated from a
  // HexColorEntry::ColorChange signal. In this way we don't override
  // what the user is writting in the text field.
  bool m_disableHexUpdate;
};

#endif
