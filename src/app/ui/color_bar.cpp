// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/color_bar.h"

#include "app/color.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/ini_file.h"
#include "app/modules/gui.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "doc/image.h"
#include "doc/palette.h"
#include "she/surface.h"
#include "ui/graphics.h"
#include "ui/menu.h"
#include "ui/paint_event.h"
#include "ui/system.h"

#include <cstring>

namespace app {

using namespace app::skin;
using namespace ui;

//////////////////////////////////////////////////////////////////////
// ColorBar::ScrollableView class

ColorBar::ScrollableView::ScrollableView()
{
  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  int l = theme->get_part(PART_EDITOR_SELECTED_W)->width();
  int t = theme->get_part(PART_EDITOR_SELECTED_N)->height();
  int r = theme->get_part(PART_EDITOR_SELECTED_E)->width();
  int b = theme->get_part(PART_EDITOR_SELECTED_S)->height();

  setBorder(gfx::Border(l, t, r, b));
}

void ColorBar::ScrollableView::onPaint(ui::PaintEvent& ev)
{
  ui::Graphics* g = ev.getGraphics();
  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  theme->draw_bounds_nw(g,
    getClientBounds(),
    hasFocus() ? PART_EDITOR_SELECTED_NW:
    PART_EDITOR_NORMAL_NW,
    gfx::ColorNone);
}

//////////////////////////////////////////////////////////////////////
// ColorBar class

ColorBar* ColorBar::m_instance = NULL;

ColorBar::ColorBar(int align)
  : Box(align)
  , m_paletteButton("Edit Palette")
  , m_paletteView(false)
  , m_fgColor(app::Color::fromRgb(255, 255, 255), IMAGE_RGB)
  , m_bgColor(app::Color::fromRgb(0, 0, 0), IMAGE_RGB)
  , m_lock(false)
{
  m_instance = this;

  setBorder(gfx::Border(2*guiscale(), 0, 0, 0));
  child_spacing = 2*guiscale();

  m_paletteView.setBoxSize(6*guiscale());
  m_paletteView.setColumns(8);
  m_fgColor.setPreferredSize(0, m_fgColor.getPreferredSize().h);
  m_bgColor.setPreferredSize(0, m_bgColor.getPreferredSize().h);

  // TODO hardcoded scroll bar width should be get from skin.xml file
  int scrollBarWidth = 6*guiscale();
  m_scrollableView.getHorizontalBar()->setBarWidth(scrollBarWidth);
  m_scrollableView.getVerticalBar()->setBarWidth(scrollBarWidth);
  setup_mini_look(m_scrollableView.getHorizontalBar());
  setup_mini_look(m_scrollableView.getVerticalBar());

  m_scrollableView.attachToView(&m_paletteView);
  m_scrollableView.setExpansive(true);

  addChild(&m_paletteButton);
  addChild(&m_scrollableView);
  addChild(&m_fgColor);
  addChild(&m_bgColor);

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
  Widget::setBgColor(((SkinTheme*)getTheme())->colors.tabActiveFace());
  m_paletteView.setBgColor(((SkinTheme*)getTheme())->colors.tabActiveFace());

  // Change labels foreground color
  setup_mini_font(m_paletteButton.mainButton());
  m_paletteButton.Click.connect(Bind<void>(&ColorBar::onPaletteButtonClick, this));
  m_paletteButton.DropDownClick.connect(Bind<void>(&ColorBar::onPaletteButtonDropDownClick, this));

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

void ColorBar::onPaletteButtonDropDownClick()
{
  if (!m_palettePopup.isVisible()) {
    gfx::Rect bounds = m_paletteButton.getBounds();

    m_palettePopup.showPopup(
      gfx::Rect(bounds.x+bounds.w, bounds.y,
        ui::display_w()/2, ui::display_h()/2));
  }
  else {
    m_palettePopup.closeWindow(NULL);
  }
}

void ColorBar::onPaletteIndexChange(PaletteIndexChangeEvent& ev)
{
  m_lock = true;

  app::Color color = app::Color::fromIndex(ev.index());

  if ((ev.buttons() & kButtonRight) == kButtonRight)
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
