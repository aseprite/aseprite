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

#include "app/app.h"
#include "app/cmd/remap_colors.h"
#include "app/cmd/set_palette.h"
#include "app/color.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/ini_file.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/transaction.h"
#include "app/ui/color_spectrum.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "doc/image.h"
#include "doc/palette.h"
#include "doc/remap.h"
#include "doc/sort_palette.h"
#include "she/surface.h"
#include "ui/graphics.h"
#include "ui/menu.h"
#include "ui/paint_event.h"
#include "ui/separator.h"
#include "ui/splitter.h"
#include "ui/system.h"

#include <cstring>

namespace app {

enum class PalButton {
  EDIT,
  SORT,
  OPTIONS,
  MAX
};

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
  , m_buttons(int(PalButton::MAX))
  , m_paletteView(true, this,
      App::instance()->preferences().colorBar.boxSize() * guiscale())
  , m_remapButton("Remap")
  , m_fgColor(app::Color::fromRgb(255, 255, 255), IMAGE_RGB)
  , m_bgColor(app::Color::fromRgb(0, 0, 0), IMAGE_RGB)
  , m_lock(false)
  , m_remap(nullptr)
  , m_lastDocument(nullptr)
  , m_ascending(true)
{
  m_instance = this;

  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());

  setBorder(gfx::Border(2*guiscale(), 0, 0, 0));
  child_spacing = 2*guiscale();

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

  m_remapButton.setVisible(false);

  ColorSpectrum* spectrum = new ColorSpectrum;
  Splitter* splitter = new Splitter(Splitter::ByPercentage, JI_VERTICAL);
  splitter->setPosition(80);
  splitter->setExpansive(true);
  splitter->addChild(&m_scrollableView);
  splitter->addChild(spectrum);

  Box* topBox = new HBox();
  topBox->addChild(&m_buttons);
  m_buttons.max_h = 15*ui::guiscale();

  addChild(topBox);
  addChild(splitter);
  addChild(&m_remapButton);
  addChild(&m_fgColor);
  addChild(&m_bgColor);

  m_remapButton.Click.connect(Bind<void>(&ColorBar::onRemapButtonClick, this));
  m_fgColor.Change.connect(&ColorBar::onFgColorButtonChange, this);
  m_bgColor.Change.connect(&ColorBar::onBgColorButtonChange, this);
  spectrum->ColorChange.connect(&ColorBar::onPickSpectrum, this);

  // Set background color reading its value from the configuration.
  setBgColor(get_config_color("ColorBar", "BG", getBgColor()));

  // Clear the selection of the BG color in the palette.
  m_paletteView.clearSelection();

  // Set foreground color reading its value from the configuration.
  setFgColor(get_config_color("ColorBar", "FG", getFgColor()));

  // Change color-bar background color (not ColorBar::setBgColor)
  Widget::setBgColor(theme->colors.tabActiveFace());
  m_paletteView.setBgColor(theme->colors.tabActiveFace());

  // Change labels foreground color
  m_buttons.ItemChange.connect(Bind<void>(&ColorBar::onPaletteButtonClick, this));

  m_buttons.addItem(theme->get_part(PART_PAL_EDIT));
  m_buttons.addItem(theme->get_part(PART_PAL_SORT));
  m_buttons.addItem(theme->get_part(PART_PAL_OPTIONS));

  onColorButtonChange(getFgColor());

  UIContext::instance()->addObserver(this);
}

ColorBar::~ColorBar()
{
  UIContext::instance()->removeObserver(this);

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
  m_buttons.getItem(int(PalButton::EDIT))->setSelected(state);
}

void ColorBar::onActiveSiteChange(const doc::Site& site)
{
  if (m_lastDocument != site.document()) {
    m_lastDocument = site.document();
    destroyRemap();
  }
}

// Switches the palette-editor
void ColorBar::onPaletteButtonClick()
{
  int item = m_buttons.selectedItem();
  m_buttons.deselectItems();

  switch (item) {

    case PalButton::EDIT: {
      Command* cmd_show_palette_editor = CommandsModule::instance()->getCommandByName(CommandId::PaletteEditor);
      Params params;
      params.set("switch", "true");

      UIContext::instance()->executeCommand(cmd_show_palette_editor, params);
      break;
    }

    case PalButton::SORT: {
      gfx::Rect bounds = m_buttons.getItem(item)->getBounds();

      Menu menu;
      MenuItem
        rev("Reverse Colors"),
        hue("Sort by Hue"),
        sat("Sort by Saturation"),
        bri("Sort by Brightness"),
        lum("Sort by Luminance"),
        asc("Ascending"),
        des("Descending");
      menu.addChild(&rev);
      menu.addChild(new ui::Separator("", JI_HORIZONTAL));
      menu.addChild(&hue);
      menu.addChild(&sat);
      menu.addChild(&bri);
      menu.addChild(&lum);
      menu.addChild(new ui::Separator("", JI_HORIZONTAL));
      menu.addChild(&asc);
      menu.addChild(&des);

      if (m_ascending) asc.setSelected(true);
      else des.setSelected(true);

      rev.Click.connect(Bind<void>(&ColorBar::onReverseColors, this));
      hue.Click.connect(Bind<void>(&ColorBar::onSortBy, this, SortPaletteBy::HUE));
      sat.Click.connect(Bind<void>(&ColorBar::onSortBy, this, SortPaletteBy::SATURATION));
      bri.Click.connect(Bind<void>(&ColorBar::onSortBy, this, SortPaletteBy::VALUE));
      lum.Click.connect(Bind<void>(&ColorBar::onSortBy, this, SortPaletteBy::LUMA));
      asc.Click.connect(Bind<void>(&ColorBar::setAscending, this, true));
      des.Click.connect(Bind<void>(&ColorBar::setAscending, this, false));

      menu.showPopup(gfx::Point(bounds.x, bounds.y+bounds.h));
      break;
    }

    case PalButton::OPTIONS: {
      if (!m_palettePopup.isVisible()) {
        gfx::Rect bounds = m_buttons.getItem(item)->getBounds();

        m_palettePopup.showPopup(
          gfx::Rect(bounds.x, bounds.y+bounds.h,
                    ui::display_w()/2, ui::display_h()/2));
      }
      else {
        m_palettePopup.closeWindow(NULL);
      }
      break;
    }

  }
}

void ColorBar::onRemapButtonClick()
{
  ASSERT(m_remap);

  try {
    ContextWriter writer(UIContext::instance(), 500);
    Sprite* sprite = writer.sprite();
    if (sprite) {
      Transaction transaction(writer.context(), "Remap Colors", ModifyDocument);
      transaction.execute(new cmd::RemapColors(sprite, *m_remap));
      transaction.commit();
    }
    update_screen_for_document(writer.document());
    destroyRemap();
  }
  catch (base::Exception& e) {
    Console::showException(e);
  }
}

void ColorBar::onPaletteViewIndexChange(int index, ui::MouseButtons buttons)
{
  m_lock = true;

  app::Color color = app::Color::fromIndex(index);

  if ((buttons & kButtonRight) == kButtonRight)
    setBgColor(color);
  else
    setFgColor(color);

  m_lock = false;
}

void ColorBar::onPaletteViewRemapColors(const Remap& remap, const Palette* newPalette)
{
  applyRemap(remap, newPalette, "Drag-and-Drop Colors");
}

void ColorBar::applyRemap(const doc::Remap& remap, const doc::Palette* newPalette, const std::string& actionText)
{
  if (!m_remap) {
    m_remap = new doc::Remap(remap);
    m_remapButton.setVisible(true);
    layout();
  }
  else {
    m_remap->merge(remap);
  }

  try {
    ContextWriter writer(UIContext::instance(), 500);
    Sprite* sprite = writer.sprite();
    frame_t frame = writer.frame();
    if (sprite) {
      Transaction transaction(writer.context(), actionText, ModifyDocument);
      transaction.execute(new cmd::SetPalette(sprite, frame, newPalette));
      transaction.commit();
    }
  }
  catch (base::Exception& e) {
    Console::showException(e);
  }
}

void ColorBar::onPaletteViewChangeSize(int boxsize)
{
  App::instance()->preferences().colorBar.boxSize(boxsize / guiscale());
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

void ColorBar::onPickSpectrum(const app::Color& color, ui::MouseButtons buttons)
{
  m_lock = true;

  if ((buttons & kButtonRight) == kButtonRight)
    setBgColor(color);
  else
    setFgColor(color);

  m_lock = false;
}

void ColorBar::onReverseColors()
{
  PaletteView::SelectedEntries entries;
  m_paletteView.getSelectedEntries(entries);

  // Count the number of selected entries.
  int n = 0;
  for (bool state : entries) {
    if (state)
      ++n;
  }

  // If there is just one selected color, we select sort them all.
  if (n < 2) {
    n = 0;
    for (auto& state : entries) {
      state = true;
      ++n;
    }
  }

  std::vector<int> mapToOriginal(n); // Maps index from selectedPalette -> palette
  int i = 0, j = 0;
  for (bool state : entries) {
    if (state)
      mapToOriginal[j++] = i;
    ++i;
  }

  Remap remap(Palette::MaxColors);
  i = 0;
  j = n;
  for (bool state : entries) {
    if (state)
      remap.map(i, mapToOriginal[--j]);
    else
      remap.map(i, i);
    ++i;
  }

  Palette newPalette(*get_current_palette(), remap);
  applyRemap(remap, &newPalette, "Reverse Colors");

  set_current_palette(&newPalette, false);
  getManager()->invalidate();
}

void ColorBar::onSortBy(SortPaletteBy channel)
{
  PaletteView::SelectedEntries entries;
  m_paletteView.getSelectedEntries(entries);

  // Count the number of selected entries.
  int n = 0;
  for (bool state : entries) {
    if (state)
      ++n;
  }

  // If there is just one selected color, we select sort them all.
  if (n < 2) {
    n = 0;
    for (auto& state : entries) {
      state = true;
      ++n;
    }
  }

  // Create a "subpalette" with selected entries only.
  Palette palette(*get_current_palette());
  Palette selectedPalette(0, n);
  std::vector<int> mapToOriginal(n); // Maps index from selectedPalette -> palette
  int i = 0, j = 0;
  for (bool state : entries) {
    if (state) {
      selectedPalette.setEntry(j, palette.getEntry(i));
      mapToOriginal[j] = i;
      ++j;
    }
    ++i;
  }

  // Create a remap to sort the selected entries with the given color
  // component/channel.
  Remap remap = doc::sort_palette(&selectedPalette, channel, m_ascending);

  // Create a bigger new remap for the original palette (with all
  // entries, selected and deselected).
  Remap remapOrig(Palette::MaxColors);
  i = j = 0;
  for (bool state : entries) {
    if (state)
      remapOrig.map(i, mapToOriginal[remap[j++]]);
    else
      remapOrig.map(i, i);
    ++i;
  }

  // Create a new palette and apply the remap. This is the final new
  // palette for the sprite.
  Palette newPalette(palette, remapOrig);

  applyRemap(remapOrig, &newPalette, "Sort Colors");

  set_current_palette(&newPalette, false);
  getManager()->invalidate();
}

void ColorBar::setAscending(bool ascending)
{
  m_ascending = ascending;
}

void ColorBar::destroyRemap()
{
  if (!m_remap)
    return;

  delete m_remap;
  m_remap = nullptr;

  m_remapButton.setVisible(false);
  layout();
}

} // namespace app
