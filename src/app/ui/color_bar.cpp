/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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
#include <cstring>

#include "app/color.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/ini_file.h"
#include "app/modules/gui.h"
#include "app/ui/color_bar.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "raster/image.h"
#include "ui/graphics.h"
#include "ui/paint_event.h"

namespace app {

using namespace app::skin;
using namespace ui;

//////////////////////////////////////////////////////////////////////
// ColorBar::ScrollableView class

ColorBar::ScrollableView::ScrollableView()
{
  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  int l = theme->get_part(PART_EDITOR_SELECTED_W)->w;
  int t = theme->get_part(PART_EDITOR_SELECTED_N)->h;
  int r = theme->get_part(PART_EDITOR_SELECTED_E)->w;
  int b = theme->get_part(PART_EDITOR_SELECTED_S)->h;

  jwidget_set_border(this, l, t, r, b);
}

void ColorBar::ScrollableView::onPaint(ui::PaintEvent& ev)
{
  ui::Graphics* g = ev.getGraphics();
  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  theme->draw_bounds_nw(g,
    getClientBounds(),
    hasFocus() ? PART_EDITOR_SELECTED_NW:
    PART_EDITOR_NORMAL_NW,
    ColorNone);
}

//////////////////////////////////////////////////////////////////////
// ColorBar class

ColorBar* ColorBar::m_instance = NULL;

ColorBar::ColorBar(int align)
  : Box(align)
  , m_paletteButton("Edit Palette", kButtonWidget)
  , m_paletteView(false)
  , m_fgColor(app::Color::fromIndex(15), IMAGE_INDEXED)
  , m_bgColor(app::Color::fromIndex(0), IMAGE_INDEXED)
  , m_lock(false)
{
  m_instance = this;

  setBorder(gfx::Border(1*jguiscale()));
  child_spacing = 1*jguiscale();

  m_paletteView.setBoxSize(6*jguiscale());
  m_paletteView.setColumns(4);
  m_fgColor.setPreferredSize(0, m_fgColor.getPreferredSize().h);
  m_bgColor.setPreferredSize(0, m_bgColor.getPreferredSize().h);

  m_scrollableView.attachToView(&m_paletteView);
  int w = (m_scrollableView.getBorder().getSize().w +
           m_scrollableView.getViewport()->getBorder().getSize().w +
           m_paletteView.getPreferredSize().w +
           getTheme()->scrollbar_size);

  jwidget_set_min_size(&m_scrollableView, w, 0);

  m_scrollableView.setExpansive(true);

  addChild(&m_paletteButton);
  addChild(&m_scrollableView);
  addChild(&m_fgColor);
  addChild(&m_bgColor);

  this->border_width.l = 2*jguiscale();
  this->border_width.t = 2*jguiscale();
  this->border_width.r = 0;
  this->border_width.b = 2*jguiscale();
  this->child_spacing = 2*jguiscale();

  m_paletteView.IndexChange.connect(&ColorBar::onPaletteIndexChange, this);
  m_fgColor.Change.connect(&ColorBar::onFgColorButtonChange, this);
  m_bgColor.Change.connect(&ColorBar::onBgColorButtonChange, this);

  // Set background color reading its value from the configuration.
  setBgColor(get_config_color("ColorBar", "BG", getBgColor()));

  // Clear the selection of the BG color in the palette.
  m_paletteView.clearSelection();

  // Set foreground color reading its value from the configuration.
  setFgColor(get_config_color("ColorBar", "FG", getFgColor()));

  // Change color-bar background color (not ColorBar::setBgColor)
  Widget::setBgColor(((SkinTheme*)getTheme())->getColor(ThemeColor::TabSelectedFace));
  m_paletteView.setBgColor(((SkinTheme*)getTheme())->getColor(ThemeColor::TabSelectedFace));

  // Change labels foreground color
  setup_mini_look(&m_paletteButton);
  m_paletteButton.setFont(((SkinTheme*)getTheme())->getMiniFont());
  m_paletteButton.Click.connect(Bind<void>(&ColorBar::onPaletteButtonClick, this));

  setDoubleBuffered(true);

  onColorButtonChange(getFgColor());
}

ColorBar::~ColorBar()
{
  set_config_color("ColorBar", "FG", getFgColor());
  set_config_color("ColorBar", "BG", getBgColor());
}

void ColorBar::setPixelFormat(PixelFormat pixelFormat)
{
  m_fgColor.setPixelFormat(pixelFormat);
  m_bgColor.setPixelFormat(pixelFormat);
}

app::Color ColorBar::getFgColor()
{
  return m_fgColor.getColor();
}

app::Color ColorBar::getBgColor()
{
  return m_bgColor.getColor();
}

void ColorBar::setFgColor(const app::Color& color)
{
  m_fgColor.setColor(color);
  FgColorChange(color);
}

void ColorBar::setBgColor(const app::Color& color)
{
  m_bgColor.setColor(color);
  BgColorChange(color);
}

PaletteView* ColorBar::getPaletteView()
{
  return &m_paletteView;
}

void ColorBar::setPaletteEditorButtonState(bool state)
{
  m_paletteButton.setSelected(state);
}

// Switches the palette-editor
void ColorBar::onPaletteButtonClick()
{
  Command* cmd_show_palette_editor = CommandsModule::instance()->getCommandByName(CommandId::PaletteEditor);
  Params params;
  params.set("switch", "true");

  UIContext::instance()->executeCommand(cmd_show_palette_editor, &params);
}

void ColorBar::onPaletteIndexChange(int index)
{
  m_lock = true;

  app::Color color = app::Color::fromIndex(index);

  // TODO create a PaletteChangeEvent and take left/right mouse button from there
  if ((jmouse_b(0) & kButtonRight) == kButtonRight)
    setBgColor(color);
  else
    setFgColor(color);

  m_lock = false;
}

void ColorBar::onFgColorButtonChange(const app::Color& color)
{
  if (!m_lock)
    m_paletteView.clearSelection();

  FgColorChange(color);
  onColorButtonChange(color);
}

void ColorBar::onBgColorButtonChange(const app::Color& color)
{
  if (!m_lock)
    m_paletteView.clearSelection();

  BgColorChange(color);
  onColorButtonChange(color);
}

void ColorBar::onColorButtonChange(const app::Color& color)
{
  if (color.getType() == app::Color::IndexType)
    m_paletteView.selectColor(color.getIndex());
}

} // namespace app
