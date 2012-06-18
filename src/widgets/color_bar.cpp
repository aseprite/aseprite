/* ASEPRITE
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

#include "config.h"

#include <allegro.h>
#include <cstring>

#include "app/color.h"
#include "base/bind.h"
#include "commands/commands.h"
#include "commands/params.h"
#include "ini_file.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "skin/skin_theme.h"
#include "ui/list.h"
#include "ui/message.h"
#include "ui_context.h"
#include "widgets/color_bar.h"
#include "widgets/statebar.h"

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

bool ColorBar::ScrollableView::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_DRAW:
      {
        Viewport* viewport = getViewport();
        Widget* child = reinterpret_cast<Widget*>(jlist_first_data(viewport->children));
        SkinTheme* theme = static_cast<SkinTheme*>(getTheme());

        theme->draw_bounds_nw(ji_screen,
                              rc->x1, rc->y1,
                              rc->x2-1, rc->y2-1,
                              hasFocus() ? PART_EDITOR_SELECTED_NW:
                                           PART_EDITOR_NORMAL_NW, false);
      }
      return true;

  }
  return View::onProcessMessage(msg);
}

//////////////////////////////////////////////////////////////////////
// ColorBar class

ColorBar::ColorBar(int align)
  : Box(align)
  , m_paletteButton("Edit Palette", JI_BUTTON)
  , m_paletteView(false)
  , m_fgColor(Color::fromIndex(15), IMAGE_INDEXED)
  , m_bgColor(Color::fromIndex(0), IMAGE_INDEXED)
  , m_lock(false)
{
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
  this->border_width.r = 2*jguiscale();
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
  Widget::setBgColor(((SkinTheme*)getTheme())->get_tab_selected_face_color());
  m_paletteView.setBgColor(((SkinTheme*)getTheme())->get_tab_selected_face_color());

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

Color ColorBar::getFgColor()
{
  return m_fgColor.getColor();
}

Color ColorBar::getBgColor()
{
  return m_bgColor.getColor();
}

void ColorBar::setFgColor(const Color& color)
{
  m_fgColor.setColor(color);
  FgColorChange(color);
}

void ColorBar::setBgColor(const Color& color)
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

  Color color = Color::fromIndex(index);

  if (jmouse_b(0) & 2) // TODO create a PaletteChangeEvent and take left/right mouse button from there
    setBgColor(color);
  else
    setFgColor(color);

  m_lock = false;
}

void ColorBar::onFgColorButtonChange(const Color& color)
{
  if (!m_lock)
    m_paletteView.clearSelection();

  FgColorChange(color);
  onColorButtonChange(color);
}

void ColorBar::onBgColorButtonChange(const Color& color)
{
  if (!m_lock)
    m_paletteView.clearSelection();

  BgColorChange(color);
  onColorButtonChange(color);
}

void ColorBar::onColorButtonChange(const Color& color)
{
  if (color.getType() == Color::IndexType)
    m_paletteView.selectColor(color.getIndex());
}
