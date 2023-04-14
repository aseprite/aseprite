// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#define COLOR_BAR_TRACE(...) // TRACE(__VA_ARGS__)

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/color_bar.h"

#include "app/app.h"
#include "app/app_menus.h"
#include "app/cmd/remap_colors.h"
#include "app/cmd/remap_tilemaps.h"
#include "app/cmd/remap_tileset.h"
#include "app/cmd/remove_tile.h"
#include "app/cmd/replace_image.h"
#include "app/cmd/set_palette.h"
#include "app/cmd/set_transparent_color.h"
#include "app/cmd_sequence.h"
#include "app/color.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/commands/quick_command.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/doc_api.h"
#include "app/doc_undo.h"
#include "app/i18n/strings.h"
#include "app/ini_file.h"
#include "app/inline_command_execution.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/tx.h"
#include "app/ui/color_spectrum.h"
#include "app/ui/color_tint_shade_tone.h"
#include "app/ui/color_wheel.h"
#include "app/ui/editor/editor.h"
#include "app/ui/hex_color_entry.h"
#include "app/ui/input_chain.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/palette_popup.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui/timeline/timeline.h"
#include "app/ui_context.h"
#include "app/ui_context.h"
#include "app/util/cel_ops.h"
#include "app/util/clipboard.h"
#include "base/scoped_value.h"
#include "doc/cel.h"
#include "doc/cels_range.h"
#include "doc/image.h"
#include "doc/image_impl.h"
#include "doc/layer_tilemap.h"
#include "doc/palette.h"
#include "doc/palette_gradient_type.h"
#include "doc/primitives.h"
#include "doc/remap.h"
#include "doc/rgbmap.h"
#include "doc/sort_palette.h"
#include "doc/sprite.h"
#include "doc/tileset.h"
#include "os/surface.h"
#include "ui/alert.h"
#include "ui/graphics.h"
#include "ui/menu.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/separator.h"
#include "ui/splitter.h"
#include "ui/system.h"
#include "ui/tooltips.h"

#include <cstring>
#include <limits>
#include <memory>

namespace app {

enum class PalButton {
  SORT,
  PRESETS,
  OPTIONS,
  MAX
};

using namespace app::skin;
using namespace ui;

class ColorBar::WarningIcon : public ui::Button {
public:
  WarningIcon() : ui::Button(std::string()) {
    initTheme();
  }
protected:
  void onInitTheme(ui::InitThemeEvent& ev) {
    ui::Button::onInitTheme(ev);

    auto theme = skin::SkinTheme::get(this);
    setStyle(theme->styles.warningBox());
  }
};

//////////////////////////////////////////////////////////////////////
// ColorBar::ScrollableView class

ColorBar::ScrollableView::ScrollableView()
{
  initTheme();
}

void ColorBar::ScrollableView::onInitTheme(InitThemeEvent& ev)
{
  auto hbar = horizontalBar();
  auto vbar = verticalBar();
  setup_mini_look(hbar);
  setup_mini_look(vbar);

  View::onInitTheme(ev);

  auto theme = SkinTheme::get(this);

  setStyle(theme->styles.colorbarView());

  hbar->setStyle(theme->styles.miniScrollbar());
  vbar->setStyle(theme->styles.miniScrollbar());
  hbar->setThumbStyle(theme->styles.miniScrollbarThumb());
  vbar->setThumbStyle(theme->styles.miniScrollbarThumb());

  const int scrollBarWidth = theme->dimensions.miniScrollbarSize();
  hbar->setBarWidth(scrollBarWidth);
  vbar->setBarWidth(scrollBarWidth);
}

//////////////////////////////////////////////////////////////////////
// ColorBar class

ColorBar* ColorBar::m_instance = NULL;

ColorBar::ColorBar(int align, TooltipManager* tooltipManager)
  : Box(align)
  , m_editPal(1)
  , m_buttons(int(PalButton::MAX))
  , m_tilesButton(1)
  , m_tilesetModeButtons(3)
  , m_splitter(Splitter::ByPercentage, VERTICAL)
  , m_splitterPalTil(Splitter::ByPercentage, VERTICAL)
  , m_paletteView(true, PaletteView::FgBgColors, this, 16)
  , m_tilesView(true, PaletteView::FgBgTiles, this, 16)
  , m_remapPalButton(Strings::color_bar_remap_palette())
  , m_remapTilesButton(Strings::color_bar_remap_tiles())
  , m_selector(ColorSelector::NONE)
  , m_tintShadeTone(nullptr)
  , m_spectrum(nullptr)
  , m_wheel(nullptr)
  , m_fgColor(app::Color::fromRgb(255, 255, 255), IMAGE_RGB, ColorBarButtonsOptions())
  , m_bgColor(app::Color::fromRgb(0, 0, 0), IMAGE_RGB, ColorBarButtonsOptions())
  , m_fgWarningIcon(new WarningIcon)
  , m_bgWarningIcon(new WarningIcon)
  , m_fromPalView(false)
  , m_fromPref(false)
  , m_fromFgButton(false)
  , m_fromBgButton(false)
  , m_lastDocument(nullptr)
  , m_lastTilesetId(doc::NullId)
  , m_ascending(true)
  , m_lastButton(kButtonLeft)
  , m_editMode(false)
  , m_tilemapMode(TilemapMode::Pixels)
  , m_tilesetMode(TilesetMode::Auto)
  , m_redrawTimer(250, this)
  , m_redrawAll(false)
  , m_implantChange(false)
  , m_selfPalChange(false)
  , m_splitterPalTilPos(50.0)
{
  m_instance = this;

  auto theme = SkinTheme::get(this);

  auto item = m_editPal.addItem("");
  item->InitTheme.connect(
    [this, item](){
      auto style =
        (m_editMode ? SkinTheme::instance()->styles.palEditUnlock() :
                      SkinTheme::instance()->styles.palEditLock());
      item->setStyle(style);
  });
  m_buttons.addItem(theme->parts.palSort(), "pal_button");
  m_buttons.addItem(theme->parts.palPresets(), "pal_button");
  m_buttons.addItem(theme->parts.palOptions(), "pal_button");
  item = m_tilesButton.addItem(theme->parts.tiles());
  item->InitTheme.connect(
    [this, item]() {
      auto theme = SkinTheme::instance();
      const bool editTiles = (canEditTiles() &&
                              m_tilemapMode == TilemapMode::Tiles);
      auto style = (editTiles ? theme->styles.editTilesMode() :
                                theme->styles.editPixelsMode());
      item->setStyle(style);
    });

  static_assert(0 == int(TilesetMode::Manual) &&
                1 == int(TilesetMode::Auto) &&
                2 == int(TilesetMode::Stack), "Tileset mode buttons doesn't match TilesetMode enum values");

  m_tilesetModeButtons.addItem(theme->parts.tilesManual(), "pal_button");
  m_tilesetModeButtons.addItem(theme->parts.tilesAuto(), "pal_button");
  m_tilesetModeButtons.addItem(theme->parts.tilesStack(), "pal_button");
  setTilesetMode(m_tilesetMode);

  m_paletteView.setColumns(8);
  m_tilesView.setColumns(8);

  m_scrollablePalView.attachToView(&m_paletteView);
  m_scrollableTilesView.attachToView(&m_tilesView);
  m_scrollablePalView.setExpansive(true);
  m_scrollableTilesView.setExpansive(true);
  m_splitterPalTil.setExpansive(true);

  m_scrollableTilesView.setVisible(false);
  m_tilesHelpers.setVisible(false);
  m_remapPalButton.setVisible(false);
  m_remapTilesButton.setVisible(false);

  m_splitterPalTil.addChild(&m_scrollablePalView);
  m_splitterPalTil.addChild(&m_scrollableTilesView);
  m_palettePlaceholder.addChild(&m_splitterPalTil);
  m_palettePlaceholder.addChild(&m_remapPalButton);
  m_palettePlaceholder.addChild(&m_remapTilesButton);
  m_splitter.setId("palette_spectrum_splitter");
  m_splitter.setPosition(80);
  m_splitter.setExpansive(true);
  m_splitter.addChild(&m_palettePlaceholder);
  m_splitter.addChild(&m_selectorPlaceholder);

  setColorSelector(
    Preferences::instance().colorBar.selector());

  m_tilesHBox.addChild(&m_tilesButton);
  m_tilesHBox.addChild(&m_tilesetModeButtons);

  m_palHBox.addChild(&m_editPal);
  m_palHBox.addChild(&m_buttons);

  m_buttons.setExpansive(true);
  m_tilesetModeButtons.setExpansive(true);

  // Hide the tiles controls by default. Without this, when the first
  // onActiveSiteChange() event is received, and the we ask for the
  // m_tilesHBox visibility, it might say that it's hidden because the
  // color bar is hidden (because it's not yet in the screen, it's the
  // first time it will be displayed). So we have to add this to make
  // the tiles controls invisible in the first appearance of the color
  // bar.
  m_tilesHBox.setVisible(false);

  addChild(&m_palHBox);
  addChild(&m_tilesHBox);
  addChild(&m_splitter);

  addChild(&m_colorHelpers);
  addChild(&m_tilesHelpers);

  HBox* fgBox = new HBox;
  HBox* bgBox = new HBox;
  fgBox->addChild(&m_fgColor);
  fgBox->addChild(m_fgWarningIcon);
  bgBox->addChild(&m_bgColor);
  bgBox->addChild(m_bgWarningIcon);
  m_colorHelpers.addChild(fgBox);
  m_colorHelpers.addChild(bgBox);

  m_tilesHelpers.addChild(new BoxFiller);
  m_tilesHelpers.addChild(&m_fgTile);
  m_tilesHelpers.addChild(&m_bgTile);
  m_tilesHelpers.addChild(new BoxFiller);

  m_fgColor.setId("fg_color");
  m_bgColor.setId("bg_color");
  m_fgColor.setExpansive(true);
  m_bgColor.setExpansive(true);

  m_remapPalButton.Click.connect([this]{ onRemapPalButtonClick(); });
  m_remapTilesButton.Click.connect([this]{ onRemapTilesButtonClick(); });
  m_fgColor.Change.connect(&ColorBar::onFgColorButtonChange, this);
  m_fgColor.BeforeChange.connect(&ColorBar::onFgColorButtonBeforeChange, this);
  m_bgColor.Change.connect(&ColorBar::onBgColorButtonChange, this);
  m_fgWarningIcon->Click.connect([this]{ onFixWarningClick(&m_fgColor, m_fgWarningIcon); });
  m_bgWarningIcon->Click.connect([this]{ onFixWarningClick(&m_bgColor, m_bgWarningIcon); });
  m_redrawTimer.Tick.connect([this]{ onTimerTick(); });
  m_editPal.ItemChange.connect([this]{ onSwitchPalEditMode(); });
  m_buttons.ItemChange.connect([this]{ onPaletteButtonClick(); });
  m_tilesButton.ItemChange.connect([this]{ onTilesButtonClick(); });
  m_tilesButton.RightClick.connect([this]{ onTilesButtonRightClick(); });
  m_fgTile.Change.connect(&ColorBar::onFgTileButtonChange, this);
  m_bgTile.Change.connect(&ColorBar::onBgTileButtonChange, this);
  m_tilesetModeButtons.ItemChange.connect([this]{ onTilesetModeButtonClick(); });

  InitTheme.connect(
    [this, fgBox, bgBox]{
      auto theme = SkinTheme::get(this);

      setBorder(gfx::Border(2*guiscale(), 0, 0, 0));
      setChildSpacing(2*guiscale());

      m_fgColor.resetSizeHint();
      m_bgColor.resetSizeHint();
      m_fgColor.setSizeHint(0, m_fgColor.sizeHint().h);
      m_bgColor.setSizeHint(0, m_bgColor.sizeHint().h);

      for (auto w : { &m_editPal, &m_buttons,
                      &m_tilesButton, &m_tilesetModeButtons }) {
        w->setMinMaxSize(
          gfx::Size(0, theme->dimensions.colorBarButtonsHeight()),
          gfx::Size(std::numeric_limits<int>::max(),
                                theme->dimensions.colorBarButtonsHeight())); // TODO add resetMaxSize
      }

      m_buttons.setMaxSize(
        gfx::Size(m_buttons.sizeHint().w,
                  theme->dimensions.colorBarButtonsHeight()));
      m_tilesetModeButtons.setMaxSize(
        gfx::Size(m_tilesetModeButtons.sizeHint().w,
                  theme->dimensions.colorBarButtonsHeight()));

      // Change color-bar background color (not ColorBar::setBgColor)
      this->Widget::setBgColor(theme->colors.tabActiveFace());
      m_paletteView.setBgColor(theme->colors.tabActiveFace());
      m_paletteView.setBoxSize(Preferences::instance().colorBar.boxSize());
      m_paletteView.initTheme();

      m_tilesView.setBgColor(theme->colors.tabActiveFace());
      m_tilesView.setBoxSize(Preferences::instance().colorBar.tilesBoxSize());
      m_tilesView.initTheme();

      // Styles
      m_splitter.setStyle(theme->styles.workspaceSplitter());
      m_splitterPalTil.setStyle(theme->styles.workspaceSplitter());

      fgBox->noBorderNoChildSpacing();
      bgBox->noBorderNoChildSpacing();

      if (m_palettePopup)
        m_palettePopup->initTheme();
    });
  initTheme();

  // Set background color reading its value from the configuration.
  setBgColor(Preferences::instance().colorBar.bgColor());

  // Clear the selection of the BG color in the palette.
  m_paletteView.deselect();

  // Set foreground color reading its value from the configuration.
  setFgColor(Preferences::instance().colorBar.fgColor());

  // Tooltips
  setupTooltips(tooltipManager);

  onColorButtonChange(getFgColor());

  UIContext::instance()->add_observer(this);
  m_beforeCmdConn = UIContext::instance()->BeforeCommandExecution.connect(&ColorBar::onBeforeExecuteCommand, this);
  m_afterCmdConn = UIContext::instance()->AfterCommandExecution.connect(&ColorBar::onAfterExecuteCommand, this);
  m_fgConn = Preferences::instance().colorBar.fgColor.AfterChange.connect([this]{ onFgColorChangeFromPreferences(); });
  m_bgConn = Preferences::instance().colorBar.bgColor.AfterChange.connect([this]{ onBgColorChangeFromPreferences(); });
  m_fgTileConn = Preferences::instance().colorBar.fgTile.AfterChange.connect([this]{ onFgTileChangeFromPreferences(); });
  m_bgTileConn = Preferences::instance().colorBar.bgTile.AfterChange.connect([this]{ onBgTileChangeFromPreferences(); });
  m_sepConn = Preferences::instance().colorBar.entriesSeparator.AfterChange.connect([this]{ invalidate(); });
  m_paletteView.FocusOrClick.connect(&ColorBar::onFocusPaletteView, this);
  m_tilesView.FocusOrClick.connect(&ColorBar::onFocusTilesView, this);
  m_appPalChangeConn = App::instance()->PaletteChange.connect(&ColorBar::onAppPaletteChange, this);
  KeyboardShortcuts::instance()->UserChange.connect(
    [this, tooltipManager]{ setupTooltips(tooltipManager); });

  setEditMode(false);
  registerCommands();
}

ColorBar::~ColorBar()
{
  UIContext::instance()->remove_observer(this);
}

void ColorBar::setPixelFormat(PixelFormat pixelFormat)
{
  m_fgColor.setPixelFormat(pixelFormat);
  m_bgColor.setPixelFormat(pixelFormat);
}

app::Color ColorBar::getFgColor() const
{
  return m_fgColor.getColor();
}

app::Color ColorBar::getBgColor() const
{
  return m_bgColor.getColor();
}

void ColorBar::setFgColor(const app::Color& color)
{
  if (m_fromFgButton)
    return;

  m_fgColor.setColor(color);
  if (!m_fromPalView)
    onColorButtonChange(color);
}

void ColorBar::setBgColor(const app::Color& color)
{
  if (m_fromBgButton)
    return;

  m_bgColor.setColor(color);
  if (!m_fromPalView)
    onColorButtonChange(color);
}

void ColorBar::setFgTile(doc::tile_t tile)
{
  m_fgTile.setTile(tile);
  m_tilesView.selectColor(tile);
  if (!m_fromPalView)
    onFgTileButtonChange(tile);
}

void ColorBar::setBgTile(doc::tile_t tile)
{
  m_bgTile.setTile(tile);
  m_tilesView.selectColor(tile);
  if (!m_fromPalView)
    onBgTileButtonChange(tile);
}

doc::tile_index ColorBar::getFgTile() const
{
  return m_fgTile.getTile();
}

doc::tile_index ColorBar::getBgTile() const
{
  return m_bgTile.getTile();
}

ColorBar::ColorSelector ColorBar::getColorSelector() const
{
  return m_selector;
}

void ColorBar::setColorSelector(ColorSelector selector)
{
  if (m_selector == selector)
    return;

  if (m_tintShadeTone) m_tintShadeTone->setVisible(false);
  if (m_spectrum) m_spectrum->setVisible(false);
  if (m_wheel) m_wheel->setVisible(false);

  m_selector = selector;
  Preferences::instance().colorBar.selector(m_selector);

  switch (m_selector) {

    case ColorSelector::TINT_SHADE_TONE:
      if (!m_tintShadeTone) {
        m_tintShadeTone = new ColorTintShadeTone;
        m_tintShadeTone->setExpansive(true);
        m_tintShadeTone->selectColor(m_fgColor.getColor());
        m_tintShadeTone->ColorChange.connect(&ColorBar::onPickSpectrum, this);
        m_selectorPlaceholder.addChild(m_tintShadeTone);
      }
      m_tintShadeTone->setVisible(true);
      break;

    case ColorSelector::SPECTRUM:
      if (!m_spectrum) {
        m_spectrum = new ColorSpectrum;
        m_spectrum->setExpansive(true);
        m_spectrum->selectColor(m_fgColor.getColor());
        m_spectrum->ColorChange.connect(&ColorBar::onPickSpectrum, this);
        m_selectorPlaceholder.addChild(m_spectrum);
      }
      m_spectrum->setVisible(true);
      break;

    case ColorSelector::RGB_WHEEL:
    case ColorSelector::RYB_WHEEL:
    case ColorSelector::NORMAL_MAP_WHEEL:
      if (!m_wheel) {
        m_wheel = new ColorWheel;
        m_wheel->setExpansive(true);
        m_wheel->selectColor(m_fgColor.getColor());
        m_wheel->ColorChange.connect(&ColorBar::onPickSpectrum, this);
        m_selectorPlaceholder.addChild(m_wheel);
      }
      if (m_selector == ColorSelector::RGB_WHEEL) {
        m_wheel->setColorModel(ColorWheel::ColorModel::RGB);
      }
      else if (m_selector == ColorSelector::RYB_WHEEL) {
        m_wheel->setColorModel(ColorWheel::ColorModel::RYB);
      }
      else if (m_selector == ColorSelector::NORMAL_MAP_WHEEL) {
        m_wheel->setColorModel(ColorWheel::ColorModel::NORMAL_MAP);
      }
      m_wheel->setVisible(true);
      break;

  }

  m_selectorPlaceholder.layout();
}

bool ColorBar::inEditMode() const
{
  return
    (m_editMode &&
     m_lastDocument &&
     m_lastDocument->sprite() &&
     m_lastDocument->sprite()->pixelFormat() != IMAGE_GRAYSCALE);
}

void ColorBar::setEditMode(bool state)
{
  m_editMode = state;

  // The item icon/style will be set depending on m_editMode state.
  ButtonSet::Item* item = m_editPal.getItem(0);
  item->initTheme();
  item->invalidate();

  // Deselect color entries when we cancel editing
  if (!state)
    m_paletteView.deselect();
}

TilemapMode ColorBar::tilemapMode() const
{
  return
    (m_lastDocument &&
     m_lastDocument->sprite()) ? m_tilemapMode:
                                 TilemapMode::Pixels;
}

void ColorBar::setTilemapMode(TilemapMode mode)
{
  // With sprites that has a custom tile management plugin, we support
  // only editing pixels in manual mode.
  if (customTileManagement()) {
    mode = TilemapMode::Pixels;
  }

  if (m_tilemapMode != mode) {
    m_tilemapMode = mode;
    updateFromTilemapMode();
  }
}

void ColorBar::updateFromTilemapMode()
{
  ButtonSet::Item* item = m_tilesButton.getItem(0);
  const bool canEditTiles = this->canEditTiles();
  const bool editTiles = (canEditTiles &&
                          m_tilemapMode == TilemapMode::Tiles);
  item->initTheme();

  if (Preferences::instance().colorBar.showColorAndTiles()) {
    m_scrollablePalView.setVisible(true);
    m_selectorPlaceholder.setVisible(true);
    if (canEditTiles) {
      m_scrollableTilesView.setVisible(true);
    }
    else {
      manager()->freeWidget(&m_tilesView);
      m_scrollableTilesView.setVisible(false);
    }

    if (editTiles) {
      m_colorHelpers.setVisible(false);
      m_tilesHelpers.setVisible(true);
    }
    else {
      m_colorHelpers.setVisible(true);
      m_tilesHelpers.setVisible(false);
    }
  }
  else {
    if (editTiles) {
      manager()->freeWidget(&m_paletteView);
      m_scrollablePalView.setVisible(false);
      m_scrollableTilesView.setVisible(true);
      m_selectorPlaceholder.setVisible(false);
      m_colorHelpers.setVisible(false);
      m_tilesHelpers.setVisible(true);
    }
    else {
      manager()->freeWidget(&m_tilesView);
      m_scrollablePalView.setVisible(true);
      m_scrollableTilesView.setVisible(false);
      m_selectorPlaceholder.setVisible(true);
      m_colorHelpers.setVisible(true);
      m_tilesHelpers.setVisible(false);
    }
  }

  layout();
}

TilesetMode ColorBar::tilesetMode() const
{
  if (m_lastDocument &&
      m_lastDocument->sprite()) {
    return m_tilesetMode;
  }
  else
    return TilesetMode::Manual;
}

void ColorBar::setTilesetMode(TilesetMode mode)
{
  // With sprites that has a custom tile management plugin, we support
  // only the manual mode.
  if (customTileManagement()) {
    mode = TilesetMode::Manual;
  }

  m_tilesetMode = mode;

  for (int i=0; i<3; ++i) {
    ButtonSet::Item* item = m_tilesetModeButtons.getItem(i);
    item->setSelected(int(mode) == i);
  }

  // Change to pixels mode automatically
  if (m_tilemapMode == TilemapMode::Tiles)
    setTilemapMode(TilemapMode::Pixels);
}

void ColorBar::onSizeHint(ui::SizeHintEvent& ev)
{
  m_colorHelpers.resetSizeHint();
  m_tilesHelpers.resetSizeHint();
  gfx::Size sz = m_colorHelpers.sizeHint();
  sz |= m_tilesHelpers.sizeHint();
  m_colorHelpers.setSizeHint(sz);
  m_tilesHelpers.setSizeHint(sz);

  Box::onSizeHint(ev);
}

void ColorBar::onActiveSiteChange(const Site& site)
{
  if (m_lastDocument != site.document()) {
    if (m_lastDocument)
      m_lastDocument->remove_observer(this);

    m_lastDocument = const_cast<Doc*>(site.document());

    if (m_lastDocument)
      m_lastDocument->add_observer(this);

    hideRemapPal();
    hideRemapTiles();
  }

  bool isTilemap = false;
  if (site.layer()) {
    isTilemap = site.layer()->isTilemap();

    if (isTilemap && customTileManagement()) {
      isTilemap = false;
      m_tilesetMode = TilesetMode::Manual;
      m_tilemapMode = TilemapMode::Pixels;
    }
  }

  if (m_tilesHBox.isVisible() != isTilemap) {
    m_tilesHBox.setVisible(isTilemap);
    updateFromTilemapMode();
  }

  if (isTilemap) {
    doc::ObjectId newTilesetId =
      static_cast<const doc::LayerTilemap*>(site.layer())->tileset()->id();
    if (m_lastTilesetId != newTilesetId) {
      m_lastTilesetId = newTilesetId;
      m_scrollableTilesView.updateView();
    }
  }
  else {
    m_lastTilesetId = doc::NullId;
  }
}

void ColorBar::onGeneralUpdate(DocEvent& ev)
{
  // TODO Observe palette changes only
  invalidate();
}

void ColorBar::onTilesetChanged(DocEvent& ev)
{
  // This can happen when a filter is applied to each tile in a
  // background thread.
  if (!ui::is_ui_thread())
    return;

  if (m_scrollableTilesView.isVisible())
    m_scrollableTilesView.updateView();

  m_tilesView.deselect();
}

void ColorBar::onTileManagementPluginChange(DocEvent& ev)
{
  // Same update process as in onActiveSiteChange()
  onActiveSiteChange(UIContext::instance()->activeSite());
}

void ColorBar::onAppPaletteChange()
{
  COLOR_BAR_TRACE("ColorBar::onAppPaletteChange()\n");

  fixColorIndex(m_fgColor);
  fixColorIndex(m_bgColor);

  updateWarningIcon(m_fgColor.getColor(), m_fgWarningIcon);
  updateWarningIcon(m_bgColor.getColor(), m_bgWarningIcon);
}

void ColorBar::onFocusPaletteView(ui::Message* msg)
{
  if (tilemapMode() != TilemapMode::Pixels)
    setTilemapMode(TilemapMode::Pixels);
  App::instance()->inputChain().prioritize(this, msg);
}

void ColorBar::onFocusTilesView(ui::Message* msg)
{
  if (tilemapMode() != TilemapMode::Tiles)
    setTilemapMode(TilemapMode::Tiles);
  App::instance()->inputChain().prioritize(this, msg);
}

void ColorBar::onBeforeExecuteCommand(CommandExecutionEvent& ev)
{
  if (ev.command()->id() == CommandId::SetPalette() ||
      ev.command()->id() == CommandId::LoadPalette() ||
      ev.command()->id() == CommandId::ColorQuantization()) {
    showRemapPal();
  }
}

void ColorBar::onAfterExecuteCommand(CommandExecutionEvent& ev)
{
  if (ev.command()->id() == CommandId::Undo() ||
      ev.command()->id() == CommandId::Redo())
    invalidate();

  // If the sprite isn't Indexed anymore (e.g. because we've just
  // undone a "RGB -> Indexed" conversion), we hide the "Remap
  // Palette" button.
  Site site = UIContext::instance()->activeSite();
  if (site.sprite() &&
      site.sprite()->pixelFormat() != IMAGE_INDEXED) {
    hideRemapPal();
  }

  // If the layer isn't a tilemap anymore, we hide the "Remap Tiles"
  // button.
  if (site.layer() &&
      !site.layer()->isTilemap()) {
    hideRemapTiles();
  }
}

void ColorBar::onSwitchPalEditMode()
{
  m_editPal.deselectItems();
  setEditMode(!inEditMode());
}

// Switches the palette-editor
void ColorBar::onPaletteButtonClick()
{
  int item = m_buttons.selectedItem();
  m_buttons.deselectItems();

  switch (static_cast<PalButton>(item)) {

    case PalButton::SORT:
      showPaletteSortOptions();
      break;

    case PalButton::PRESETS:
      showPalettePresets();
      break;

    case PalButton::OPTIONS:
      showPaletteOptions();
      break;

  }
}

void ColorBar::onTilesButtonClick()
{
  m_tilesButton.deselectItems();
  setTilemapMode(
    (tilemapMode() == TilemapMode::Pixels ? TilemapMode::Tiles:
                                            TilemapMode::Pixels));
}

void ColorBar::onTilesButtonRightClick()
{
  auto& pref = Preferences::instance();
  pref.colorBar.showColorAndTiles(!pref.colorBar.showColorAndTiles());
  updateFromTilemapMode();
}

void ColorBar::onTilesetModeButtonClick()
{
  int item = m_tilesetModeButtons.selectedItem();
  m_tilesetModeButtons.deselectItems();
  setTilesetMode(static_cast<TilesetMode>(item));
}

void ColorBar::onRemapPalButtonClick()
{
  ASSERT(m_oldPalette);

  // Create remap from m_oldPalette to the current palette
  Remap remap(1);
  try {
    InlineCommandExecution inlineCmd(UIContext::instance());
    ContextWriter writer(UIContext::instance());
    Sprite* sprite = writer.sprite();
    ASSERT(sprite);
    if (!sprite)
      return;

    remap = create_remap_to_change_palette(
      m_oldPalette.get(), get_current_palette(),
      sprite->transparentColor(), true);
  }
  catch (base::Exception& e) {
    Console::showException(e);
  }

  // Check the remap
  if (!remap.isFor8bit() &&
      Alert::show(Strings::alerts_auto_remap()) != 1) {
    return;
  }

  try {
    InlineCommandExecution inlineCmd(UIContext::instance());
    ContextWriter writer(UIContext::instance());
    Sprite* sprite = writer.sprite();
    if (sprite) {
      ASSERT(sprite->pixelFormat() == IMAGE_INDEXED);

      Tx tx(writer.context(), "Remap Colors", ModifyDocument);
      bool remapPixels = true;

      std::vector<ImageRef> images;
      sprite->getImages(images);

      if (remap.isFor8bit()) {
        PalettePicks usedEntries(256);

        for (ImageRef& image : images) {
          for (const auto& i : LockImageBits<IndexedTraits>(image.get()))
            usedEntries[i] = true;
        }

        if (remap.isInvertible(usedEntries)) {
          for (int i=0; i<remap.size(); ++i) {
            if (i >= usedEntries.size() || !usedEntries[i]) {
              remap.unused(i);
            }
          }

          tx(new cmd::RemapColors(sprite, remap));
          remapPixels = false;
        }
      }

      // Special remap saving original images in undo history
      if (remapPixels) {
        for (ImageRef& image : images) {
          ImageRef newImage(Image::createCopy(image.get()));
          doc::remap_image(newImage.get(), remap);

          tx(new cmd::ReplaceImage(sprite, image, newImage));
        }
      }

      color_t oldTransparent = sprite->transparentColor();
      color_t newTransparent = (remap[oldTransparent] >= 0) ? remap[oldTransparent] : oldTransparent;
      if (newTransparent >= get_current_palette()->size())
        newTransparent = get_current_palette()->size() - 1;
      if (oldTransparent != newTransparent)
        tx(new cmd::SetTransparentColor(sprite, newTransparent));

      tx.commit();
    }
    update_screen_for_document(writer.document());
    hideRemapPal();
  }
  catch (base::Exception& e) {
    Console::showException(e);
  }
}

void ColorBar::onRemapTilesButtonClick()
{
  COLOR_BAR_TRACE("remapTiles\n");
  try {
    InlineCommandExecution inlineCmd(UIContext::instance());
    ContextWriter writer(UIContext::instance(), 500);
    Sprite* sprite = writer.sprite();
    if (!sprite)
      return;

    doc::Tileset* tileset = m_tilesView.tileset();
    std::vector<ImageRef> tilemaps;
    sprite->getTilemapsByTileset(tileset, tilemaps);

    const int n = std::max(m_oldTileset->size(),
                           tileset->size());
    PalettePicks usedTiles(n);

    if (n > 0) {
      for (const ImageRef& tilemap : tilemaps) {
        for (const doc::tile_t t : LockImageBits<TilemapTraits>(tilemap.get()))
          if (t != doc::notile)
            usedTiles[doc::tile_geti(t)] = true;
      }
    }

    // Remap all tiles in the same order as in newTileset
    Remap remap(n);
    bool existMapToEmpty = false;

    for (tile_index ti=0; ti<n; ++ti) {
      auto img = m_oldTileset->get(ti);
      tile_index destTi;
      if (img) {
        tileset->findTileIndex(img, destTi);

        COLOR_BAR_TRACE(" - Remap tile %d -> %d\n", ti, destTi);
        remap.map(ti, destTi);
      }
      else {
        remap.map(ti, destTi = doc::notile);
      }

      if (destTi == doc::notile &&
          ti < usedTiles.size() && usedTiles[ti]) {
        COLOR_BAR_TRACE(" - Remap tile %d to empty (used=%d)\n", ti, usedTiles[ti]);
        existMapToEmpty = true;
      }
    }

    // Nothing to remap
    if (remap.isIdentity()) {
      COLOR_BAR_TRACE(" - Nothing to remap\n");
      return;
    }

    Tx tx(writer.context(), Strings::color_bar_remap_tiles(), ModifyDocument);
    if (!existMapToEmpty &&
        remap.isInvertible(usedTiles)) {
      tx(new cmd::RemapTilemaps(tileset, remap));
    }
    else {
      for (const ImageRef& tilemap : tilemaps) {
        ImageRef newTilemap(Image::createCopy(tilemap.get()));
        doc::remap_image(newTilemap.get(), remap);

        // TODO improve this with a cmd::CopyRegion()
        tx(new cmd::ReplaceImage(sprite, tilemap, newTilemap));
      }
    }
    tx.commit();

    hideRemapTiles();
    // TODO this should be automatic in last ~Tx() destruction
    manager()->invalidate();
  }
  catch (base::Exception& e) {
    Console::showException(e);
  }
}

bool ColorBar::onIsPaletteViewActive(PaletteView* paletteView) const
{
  if (paletteView == &m_paletteView) {
    return
      (m_tilemapMode == TilemapMode::Pixels) ||
      // As the m_tilemapMode value is kept to restore it if we go
      // back to a tilemap layer, there is a possibility where we are
      // in a regular layer with the tiles mode enabled (because we
      // were in a tilemap layer just right before). We have to check
      // this special case where we are in "tiles mode" but we are
      // actually in a regular layer (canEditTiles() is false).
      (m_tilemapMode == TilemapMode::Tiles && !canEditTiles());
  }
  else if (paletteView == &m_tilesView) {
    return (m_tilemapMode == TilemapMode::Tiles);
  }
  else {
    return false;
  }
}

void ColorBar::onPaletteViewIndexChange(int index, ui::MouseButton button)
{
  COLOR_BAR_TRACE("ColorBar::onPaletteViewIndexChange(%d)\n", index);

  base::ScopedValue lock(m_fromPalView, true);

  app::Color color = app::Color::fromIndex(index);

  if (button == kButtonRight)
    setBgColor(color);
  else if (button == kButtonLeft)
    setFgColor(color);
  else if (button == kButtonMiddle)
    setTransparentIndex(index);

  ChangeSelection();
}

void ColorBar::onPaletteViewModification(const Palette* newPalette,
                                         PaletteViewModification mod)
{
  const char* text = "Palette Change";
  switch (mod) {
    case PaletteViewModification::CLEAR: text = "Clear Colors"; break;
    case PaletteViewModification::DRAGANDDROP: text = "Drag-and-Drop Colors"; break;
    case PaletteViewModification::RESIZE: text = "Resize Palette"; break;
  }
  setPalette(newPalette, text);
}

void ColorBar::setPalette(const doc::Palette* newPalette, const std::string& actionText)
{
  showRemapPal();

  try {
    InlineCommandExecution inlineCmd(UIContext::instance());
    ContextWriter writer(UIContext::instance(), 500);
    Sprite* sprite = writer.sprite();
    frame_t frame = writer.frame();
    if (sprite &&
        newPalette->countDiff(sprite->palette(frame), nullptr, nullptr)) {
      Tx tx(writer.context(), actionText, ModifyDocument);
      tx(new cmd::SetPalette(sprite, frame, newPalette));
      tx.commit();
    }
  }
  catch (base::Exception& e) {
    Console::showException(e);
  }
}

void ColorBar::setTransparentIndex(int index)
{
  try {
    InlineCommandExecution inlineCmd(UIContext::instance());
    ContextWriter writer(UIContext::instance());
    Sprite* sprite = writer.sprite();
    if (sprite &&
        sprite->pixelFormat() == IMAGE_INDEXED &&
        int(sprite->transparentColor()) != index) {
      // TODO merge this code with SpritePropertiesCommand
      Tx tx(writer.context(), "Set Transparent Color");
      DocApi api = writer.document()->getApi(tx);
      api.setSpriteTransparentColor(sprite, index);
      tx.commit();

      update_screen_for_document(writer.document());
    }
  }
  catch (base::Exception& e) {
    Console::showException(e);
  }
}

void ColorBar::onPaletteViewChangeSize(PaletteView* paletteView, int boxsize)
{
  if (paletteView == &m_tilesView)
    Preferences::instance().colorBar.tilesBoxSize(boxsize);
  else
    Preferences::instance().colorBar.boxSize(boxsize);
}

void ColorBar::onPaletteViewPasteColors(
  const Palette* fromPal, const doc::PalettePicks& from, const doc::PalettePicks& _to)
{
  if (!from.picks() || !_to.picks()) // Nothing to do
    return;

  doc::PalettePicks to = _to;
  int to_first = to.firstPick();
  int to_last = to.lastPick();

  // Add extra picks in to range if it's needed to paste more colors.
  int from_picks = from.picks();
  int to_picks = to.picks();
  if (to_picks < from_picks) {
    for (int j=to_last+1; j<to.size() && to_picks<from_picks; ++j) {
      to[j] = true;
      ++to_picks;
    }
  }

  Palette newPalette(*get_current_palette());

  int i = 0;
  int j = to_first;

  for (auto state : from) {
    if (state) {
      if (j < newPalette.size())
        newPalette.setEntry(j, fromPal->getEntry(i));
      else
        newPalette.addEntry(fromPal->getEntry(i));

      for (++j; j<to.size(); ++j)
        if (to[j])
          break;
    }
    ++i;
  }

  setPalette(&newPalette, "Paste Colors");
}

app::Color ColorBar::onPaletteViewGetForegroundIndex()
{
  return getFgColor();
}

app::Color ColorBar::onPaletteViewGetBackgroundIndex()
{
  return getBgColor();
}

doc::tile_index ColorBar::onPaletteViewGetForegroundTile()
{
  return doc::tile_geti(getFgTile());
}

doc::tile_index ColorBar::onPaletteViewGetBackgroundTile()
{
  return doc::tile_geti(getBgTile());
}

void ColorBar::onTilesViewClearTiles(const doc::PalettePicks& _picks)
{
  // Copy the collection of selected tiles because in case that the
  // user want to delete a range of tiles (several tiles at the same
  // time), after the first cmd::RemoveTile() is executed this
  // collection (_picks) will be modified and we'll lost the other
  // selected tiles to remove.
  doc::PalettePicks picks = _picks;
  try {
    InlineCommandExecution inlineCmd(UIContext::instance());
    ContextWriter writer(UIContext::instance(), 500);
    Sprite* sprite = writer.sprite();
    ASSERT(writer.layer()->isTilemap());
    if (sprite) {
      auto tileset = m_tilesView.tileset();

      Tx tx(writer.context(), "Clear Tiles", ModifyDocument);
      for (int ti=int(picks.size())-1; ti>=0; --ti) {
        if (picks[ti])
          tx(new cmd::RemoveTile(tileset, ti));
      }
      tx.commit();

      update_screen_for_document(writer.document());
    }
  }
  catch (base::Exception& e) {
    Console::showException(e);
  }
}

void ColorBar::onTilesViewResize(const int newSize)
{
  auto tileset = m_tilesView.tileset();
  if (!tileset || tileset->size() == newSize)
    return;

  showRemapTiles();

  try {
    InlineCommandExecution inlineCmd(UIContext::instance());
    ContextWriter writer(UIContext::instance(), 500);
    Sprite* sprite = writer.sprite();
    ASSERT(writer.layer()->isTilemap());
    if (sprite) {
      auto tileset = m_tilesView.tileset();

      Tx tx(writer.context(), Strings::color_bar_resize_tiles(), ModifyDocument);
      if (tileset->size() < newSize) {
        for (doc::tile_index ti=tileset->size(); ti<newSize; ++ti) {
          ImageRef img = tileset->makeEmptyTile();
          tx(new cmd::AddTile(tileset, img));
        }
      }
      else {
        for (doc::tile_index ti=tileset->size()-1;
             ti!=(doc::tile_index)newSize-1; --ti) {
          tx(new cmd::RemoveTile(tileset, ti));
        }
      }

      tx.commit();

      // TODO this should be automatic (when tileset is changed after a transaction)
      m_scrollableTilesView.updateView();
      update_screen_for_document(writer.document());
    }
  }
  catch (base::Exception& e) {
    Console::showException(e);
  }
}

void ColorBar::onTilesViewDragAndDrop(doc::Tileset* tileset,
                                      doc::PalettePicks& picks,
                                      int& currentEntry,
                                      const int beforeIndex,
                                      const bool isCopy)
{
  COLOR_BAR_TRACE("ColorBar::onTilesViewDragAndDrop() -> beforeIndex=%d\n",
                  beforeIndex);

  showRemapTiles();

  try {
    Context* ctx = UIContext::instance();
    InlineCommandExecution inlineCmd(ctx);
    ContextWriter writer(ctx, 500);
    Tx tx(writer.context(), Strings::color_bar_drag_and_drop_tiles(), ModifyDocument);
    if (isCopy)
      copy_tiles_in_tileset(tx, tileset, picks, currentEntry, beforeIndex);
    else
      move_tiles_in_tileset(tx, tileset, picks, currentEntry, beforeIndex);
    tx.commit();

    m_scrollableTilesView.updateView();
    update_screen_for_document(writer.document());

    ctx->setSelectedTiles(picks);
  }
  catch (base::Exception& e) {
    Console::showException(e);
  }
}

void ColorBar::onTilesViewIndexChange(int index, ui::MouseButton button)
{
  auto& pref = Preferences::instance();
  if (button == kButtonRight)
    pref.colorBar.bgTile(doc::tile(index, 0));
  else if (button == kButtonLeft)
    pref.colorBar.fgTile(doc::tile(index, 0));
  else if (button == kButtonMiddle) {
    // TODO ?
  }
}

void ColorBar::onFgColorChangeFromPreferences()
{
  COLOR_BAR_TRACE("ColorBar::onFgColorChangeFromPreferences() -> %s\n",
                  Preferences::instance().colorBar.fgColor().toString().c_str());

  if (m_fromPref)
    return;

  base::ScopedValue sync(m_fromPref, true);
  setFgColor(Preferences::instance().colorBar.fgColor());
}

void ColorBar::onBgColorChangeFromPreferences()
{
  COLOR_BAR_TRACE("ColorBar::onBgColorChangeFromPreferences() -> %s\n",
                  Preferences::instance().colorBar.bgColor().toString().c_str());

  if (m_fromPref)
    return;

  if (inEditMode()) {
    // In edit mode, clicking with right-click will copy the color
    // selected with eyedropper to the active color entry.
    setFgColor(Preferences::instance().colorBar.bgColor());
  }
  else {
    base::ScopedValue sync(m_fromPref, true);
    setBgColor(Preferences::instance().colorBar.bgColor());
  }
}

void ColorBar::onFgTileChangeFromPreferences()
{
  if (m_fromPref)
    return;

  base::ScopedValue sync(m_fromPref, true);
  auto tile = Preferences::instance().colorBar.fgTile();
  m_fgTile.setTile(tile);
  m_tilesView.selectColor(tile);
}

void ColorBar::onBgTileChangeFromPreferences()
{
  if (m_fromPref)
    return;

  base::ScopedValue sync(m_fromPref, true);
  auto tile = Preferences::instance().colorBar.bgTile();
  m_bgTile.setTile(tile);
  m_tilesView.selectColor(tile);
}

void ColorBar::onFgColorButtonBeforeChange(app::Color& color)
{
  COLOR_BAR_TRACE("ColorBar::onFgColorButtonBeforeChange(%s)\n", color.toString().c_str());

  if (m_fromPalView)
    return;

  if (!inEditMode() || color.getType() == app::Color::IndexType) {
    m_paletteView.deselect();
    return;
  }

  // Here we change the selected colors in the
  // palette. "m_fromPref" must be false to edit the color. (It
  // means, if the eyedropper was used with the left-click, we don't
  // edit the color, we just select the color to as the normal
  // non-edit mode.)
  if (!m_fromPref) {
    int i = setPaletteEntry(color);
    if (i >= 0) {
      updateCurrentSpritePalette("Color Change");
      color = app::Color::fromIndex(i);
    }
  }
}

void ColorBar::onFgColorButtonChange(const app::Color& color)
{
  COLOR_BAR_TRACE("ColorBar::onFgColorButtonChange(%s)\n", color.toString().c_str());

  if (m_fromFgButton)
    return;

  base::ScopedValue lock(m_fromFgButton, true);

  if (!m_fromPref) {
    base::ScopedValue sync(m_fromPref, true);
    Preferences::instance().colorBar.fgColor(color);
  }

  updateWarningIcon(color, m_fgWarningIcon);
  onColorButtonChange(color);
}

void ColorBar::onBgColorButtonChange(const app::Color& color)
{
  COLOR_BAR_TRACE("ColorBar::onBgColorButtonChange(%s)\n", color.toString().c_str());

  if (m_fromBgButton)
    return;

  base::ScopedValue lock(m_fromBgButton, true);

  if (!m_fromPalView && !inEditMode())
    m_paletteView.deselect();

  if (!m_fromPref) {
    base::ScopedValue sync(m_fromPref, true);
    Preferences::instance().colorBar.bgColor(color);
  }

  updateWarningIcon(color, m_bgWarningIcon);
  onColorButtonChange(color);
}

void ColorBar::onColorButtonChange(const app::Color& color)
{
  COLOR_BAR_TRACE("ColorBar::onColorButtonChange(%s)\n", color.toString().c_str());

  if (!inEditMode() || color.getType() == app::Color::IndexType || m_fromPref) {
    if (color.getType() == app::Color::IndexType)
      m_paletteView.selectColor(color.getIndex());
    else {
      m_paletteView.selectExactMatchColor(color);

      // As foreground or background color changed, we've to redraw the
      // palette view fg/bg indicators.
      m_paletteView.invalidate();
    }
  }

  if (m_tintShadeTone && m_tintShadeTone->isVisible())
    m_tintShadeTone->selectColor(color);

  if (m_spectrum && m_spectrum->isVisible())
    m_spectrum->selectColor(color);

  if (m_wheel && m_wheel->isVisible())
    m_wheel->selectColor(color);
}

void ColorBar::onFgTileButtonChange(doc::tile_t tile)
{
  if (!m_fromPref) {
    Preferences::instance().colorBar.fgTile(tile);
    m_tilesView.deselect();
  }
}

void ColorBar::onBgTileButtonChange(doc::tile_t tile)
{
  if (!m_fromPref) {
    Preferences::instance().colorBar.bgTile(tile);
    m_tilesView.deselect();
  }
}

void ColorBar::onPickSpectrum(const app::Color& color, ui::MouseButton button)
{
  // Change to pixels mode automatically
  if (m_tilemapMode == TilemapMode::Tiles)
    setTilemapMode(TilemapMode::Pixels);

  if (button == kButtonNone)
    button = m_lastButton;

  if (button == kButtonRight)
    setBgColor(color);
  else if (button == kButtonLeft)
    setFgColor(color);

  m_lastButton = button;
}

void ColorBar::onReverseColors()
{
  doc::PalettePicks entries;
  m_paletteView.getSelectedEntries(entries);

  entries.pickAllIfNeeded();
  int n = entries.picks();

  std::vector<int> mapToOriginal(n); // Maps index from selectedPalette -> palette
  int i = 0, j = 0;
  for (bool state : entries) {
    if (state)
      mapToOriginal[j++] = i;
    ++i;
  }

  Remap remap(get_current_palette()->size());
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
  setPalette(&newPalette, Strings::color_bar_reverse_colors());
}

void ColorBar::onSortBy(SortPaletteBy channel)
{
  PalettePicks entries;
  m_paletteView.getSelectedEntries(entries);

  entries.pickAllIfNeeded();
  int n = entries.picks();

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
  Remap remapOrig(palette.size());
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
  setPalette(&newPalette, Strings::color_bar_sort_colors());
}

void ColorBar::onGradient(GradientType gradientType)
{
  int index1, index2;
  if (!m_paletteView.getSelectedRange(index1, index2))
    return;

  Palette newPalette(*get_current_palette());
  if (gradientType == GradientType::LINEAR) {
    newPalette.makeGradient(index1, index2);
    setPalette(&newPalette, Strings::color_bar_gradient());
  }
  else {
    newPalette.makeHueGradient(index1, index2);
    setPalette(&newPalette, Strings::color_bar_gradient_by_hue());
  }
}

void ColorBar::setAscending(bool ascending)
{
  m_ascending = ascending;
}

void ColorBar::showRemapPal()
{
  Site site = UIContext::instance()->activeSite();
  if (site.sprite() &&
      site.sprite()->pixelFormat() == IMAGE_INDEXED) {
    if (!m_oldPalette) {
      m_oldPalette.reset(new Palette(*get_current_palette()));
      m_remapPalButton.setVisible(true);
      layout();
    }
  }
}

void ColorBar::showRemapTiles()
{
  Site site = UIContext::instance()->activeSite();
  if (site.layer() &&
      site.layer()->isTilemap()) {
    if (!m_oldTileset) {
      m_oldTileset.reset(
        Tileset::MakeCopyCopyingImages(
          static_cast<LayerTilemap*>(site.layer())->tileset()));
      m_remapTilesButton.setVisible(true);
      layout();
    }
  }
}

void ColorBar::hideRemapPal()
{
  if (!m_oldPalette)
    return;

  m_oldPalette.reset();
  m_remapPalButton.setVisible(false);
  layout();
}

void ColorBar::hideRemapTiles()
{
  if (!m_oldTileset)
    return;

  m_oldTileset.reset();
  m_remapTilesButton.setVisible(false);
  layout();
}

void ColorBar::onNewInputPriority(InputChainElement* element,
                                  const ui::Message* msg)
{
  if (dynamic_cast<Timeline*>(element) &&
      msg && (msg->ctrlPressed() || msg->shiftPressed()))
    return;

  if (element != this) {
    m_paletteView.deselect();
    m_tilesView.deselect();
  }
}

bool ColorBar::onCanCut(Context* ctx)
{
  if (m_tilemapMode == TilemapMode::Tiles)
    return (m_tilesView.getSelectedEntriesCount() > 0);
  else
    return (m_paletteView.getSelectedEntriesCount() > 0);
}

bool ColorBar::onCanCopy(Context* ctx)
{
  return onCanCut(ctx);
}

bool ColorBar::onCanPaste(Context* ctx)
{
  auto format = ctx->clipboard()->format();
  if (m_tilemapMode == TilemapMode::Tiles)
    return (format == ClipboardFormat::Tileset);
  else
    return (format == ClipboardFormat::PaletteEntries);
}

bool ColorBar::onCanClear(Context* ctx)
{
  return onCanCut(ctx);
}

bool ColorBar::onCut(Context* ctx)
{
  if (m_tilemapMode == TilemapMode::Tiles) {
    showRemapTiles();
    m_tilesView.cutToClipboard();
  }
  else
    m_paletteView.cutToClipboard();
  return true;
}

bool ColorBar::onCopy(Context* ctx)
{
  if (m_tilemapMode == TilemapMode::Tiles)
    m_tilesView.copyToClipboard();
  else
    m_paletteView.copyToClipboard();
  return true;
}

bool ColorBar::onPaste(Context* ctx)
{
  if (m_tilemapMode == TilemapMode::Tiles) {
    showRemapTiles();
    m_tilesView.pasteFromClipboard();
  }
  else
    m_paletteView.pasteFromClipboard();
  return true;
}

bool ColorBar::onClear(Context* ctx)
{
  if (m_tilemapMode == TilemapMode::Tiles) {
    showRemapTiles();
    m_tilesView.clearSelection();
  }
  else
    m_paletteView.clearSelection();
  return true;
}

void ColorBar::onCancel(Context* ctx)
{
  m_tilesView.deselect();
  m_tilesView.discardClipboardSelection();
  m_paletteView.deselect();
  m_paletteView.discardClipboardSelection();
  invalidate();
}

void ColorBar::onFixWarningClick(ColorButton* colorButton, ui::Button* warningIcon)
{
  COLOR_BAR_TRACE("ColorBar::onFixWarningClick(%s)\n", colorButton->getColor().toString().c_str());

  Palette* palette = get_current_palette();
  const int oldEntries = palette->size();

  Command* command = Commands::instance()->byId(CommandId::AddColor());
  Params params;
  params.set("source", "color");
  params.set("color", colorButton->getColor().toString().c_str());
  UIContext::instance()->executeCommand(command, params);

  // Select the new FG/BG color as an indexed color
  if (inEditMode()) {
    const int newEntries = palette->size();
    if (oldEntries != newEntries) {
      base::ScopedValue sync(m_fromPref, true);
      app::Color newIndex = app::Color::fromIndex(newEntries-1);
      if (colorButton == &m_bgColor)
        setBgColor(newIndex);
      setFgColor(newIndex);
    }
  }
}

void ColorBar::onTimerTick()
{
  // Redraw all editors
  if (m_redrawAll) {
    m_redrawAll = false;
    m_implantChange = false;
    m_redrawTimer.stop();

    // Call all observers of PaletteChange event.
    m_selfPalChange = true;
    App::instance()->PaletteChange();
    m_selfPalChange = false;

    // Redraw all editors
    try {
      InlineCommandExecution inlineCmd(UIContext::instance());
      ContextWriter writer(UIContext::instance());
      Doc* document(writer.document());
      if (document != NULL)
        document->notifyGeneralUpdate();
    }
    catch (...) {
      // Do nothing
    }
  }
  // Redraw just the current editor
  else {
    m_redrawAll = true;
    if (auto editor = Editor::activeEditor())
      editor->updateEditor(true);
  }
}

void ColorBar::updateWarningIcon(const app::Color& color, ui::Button* warningIcon)
{
  int index = -1;

  if (color.getType() == app::Color::MaskType) {
    auto editor = Editor::activeEditor();
    if (editor &&
        editor->sprite()) {
      index = editor->sprite()->transparentColor();
    }
    else
      index = 0;
  }
  else {
    index = get_current_palette()->findExactMatch(
      color.getRed(),
      color.getGreen(),
      color.getBlue(),
      color.getAlpha(), -1);
  }

  warningIcon->setVisible(index < 0);
  warningIcon->parent()->layout();
}

// Changes the selected color palettes with the given
// app::Color. Returns the first modified index in the palette.
int ColorBar::setPaletteEntry(const app::Color& color)
{
  int selIdx = m_paletteView.getSelectedEntry();
  if (selIdx < 0) {
    if (getFgColor().getType() == app::Color::IndexType) {
      selIdx = getFgColor().getIndex();
    }
  }

  PalettePicks entries;
  m_paletteView.getSelectedEntries(entries);
  if (entries.picks() == 0) {
    if (selIdx >= 0 && selIdx < entries.size()) {
      entries[selIdx] = true;
    }
  }

  doc::color_t c =
    doc::rgba(color.getRed(), color.getGreen(), color.getBlue(), color.getAlpha());

  Palette* palette = get_current_palette();
  for (int i=0; i<palette->size(); ++i) {
    if (entries[i])
      palette->setEntry(i, c);
  }

  if (selIdx < 0 ||
      selIdx >= entries.size() ||
      !entries[selIdx])
    selIdx = entries.firstPick();

  return selIdx;
}

void ColorBar::updateCurrentSpritePalette(const char* operationName)
{
  if (UIContext::instance()->activeDocument() &&
      UIContext::instance()->activeDocument()->sprite()) {
    try {
      InlineCommandExecution inlineCmd(UIContext::instance());
      ContextWriter writer(UIContext::instance());
      Doc* document(writer.document());
      Sprite* sprite(writer.sprite());
      Palette* newPalette = get_current_palette(); // System current pal
      frame_t frame = writer.frame();
      Palette* currentSpritePalette = sprite->palette(frame); // Sprite current pal
      int from, to;

      // Check differences between current sprite palette and current system palette
      from = to = -1;
      currentSpritePalette->countDiff(newPalette, &from, &to);

      if (from >= 0 && to >= from) {
        DocUndo* undo = document->undoHistory();
        std::unique_ptr<cmd::SetPalette> cmd(
          new cmd::SetPalette(sprite, frame, newPalette));

        // Add undo information to save the range of pal entries that will be modified.
        if (m_implantChange &&
            undo->lastExecutedCmd() &&
            undo->lastExecutedCmd()->label() == operationName) {
          // Implant the cmd in the last CmdSequence if it's
          // related about color palette modifications
          ASSERT(dynamic_cast<CmdSequence*>(undo->lastExecutedCmd()));
          static_cast<CmdSequence*>(undo->lastExecutedCmd())->add(cmd.get());
          // Release the unique pointer because it's already in the
          // last executed command CmdSequence::m_cmds container, and
          // execute it.
          cmd.release()->execute(UIContext::instance());
        }
        else {
          Tx tx(writer.context(), operationName, ModifyDocument);
          // If tx() fails it will delete the cmd anyway, so we can
          // release the unique pointer here.
          tx(cmd.release());
          tx.commit();
        }
      }
    }
    catch (base::Exception& e) {
      Console::showException(e);
    }
  }

  m_paletteView.invalidate();

  if (!m_redrawTimer.isRunning())
    m_redrawTimer.start();

  m_redrawAll = false;
  m_implantChange = true;
}

void ColorBar::setupTooltips(TooltipManager* tooltipManager)
{
  tooltipManager->addTooltipFor(&m_fgColor, Strings::color_bar_fg(), LEFT);
  tooltipManager->addTooltipFor(&m_bgColor, Strings::color_bar_bg(), LEFT);
  tooltipManager->addTooltipFor(m_fgWarningIcon, Strings::color_bar_fg_warning(), LEFT);
  tooltipManager->addTooltipFor(m_bgWarningIcon, Strings::color_bar_bg_warning(), LEFT);

  tooltipManager->addTooltipFor(
    m_editPal.getItem(0),
    key_tooltip(Strings::color_bar_edit_color().c_str(), CommandId::PaletteEditor()),
    BOTTOM);

  tooltipManager->addTooltipFor(m_buttons.getItem((int)PalButton::SORT), Strings::color_bar_sort_and_gradients(), BOTTOM);
  tooltipManager->addTooltipFor(m_buttons.getItem((int)PalButton::PRESETS), Strings::color_bar_presets(), BOTTOM);
  tooltipManager->addTooltipFor(m_buttons.getItem((int)PalButton::OPTIONS), Strings::color_bar_options(), BOTTOM);
  tooltipManager->addTooltipFor(&m_remapPalButton, Strings::color_bar_remap_palette_tooltip(), BOTTOM);
  tooltipManager->addTooltipFor(&m_remapTilesButton, Strings::color_bar_remap_tiles_tooltip(), BOTTOM);

  tooltipManager->addTooltipFor(
    m_tilesButton.getItem(0),
    key_tooltip(Strings::color_bar_switch_tileset().c_str(), CommandId::ToggleTilesMode()),
    BOTTOM);
  Command* cmd = Commands::instance()->byId(CommandId::TilesetMode());
  Params params;
  params.set("mode", "manual");
  tooltipManager->addTooltipFor(
    m_tilesetModeButtons.getItem((int)TilesetMode::Manual),
    key_tooltip(Strings::color_bar_tileset_mode_manual().c_str(), cmd->id().c_str(), params), BOTTOM);
  params.set("mode", "auto");
  tooltipManager->addTooltipFor(
    m_tilesetModeButtons.getItem((int)TilesetMode::Auto),
    key_tooltip(Strings::color_bar_tileset_mode_auto().c_str(), cmd->id().c_str(), params), BOTTOM);
  params.set("mode", "stack");
  tooltipManager->addTooltipFor(
    m_tilesetModeButtons.getItem((int)TilesetMode::Stack),
    key_tooltip(Strings::color_bar_tileset_mode_stack().c_str(), cmd->id().c_str(), params), BOTTOM);
}

// static
void ColorBar::fixColorIndex(ColorButton& colorButton)
{
  app::Color color = colorButton.getColor();

  if (color.getType() == Color::IndexType) {
    int oldIndex = color.getIndex();
    int newIndex = std::clamp(oldIndex, 0, get_current_palette()->size()-1);
    if (oldIndex != newIndex) {
      color = Color::fromIndex(newIndex);
      colorButton.setColor(color);
    }
  }
}

void ColorBar::registerCommands()
{
  Commands::instance()
    ->add(
      new QuickCommand(
        CommandId::ShowPaletteSortOptions(),
        [this]{ this->showPaletteSortOptions(); }))
    ->add(
      new QuickCommand(
        CommandId::ShowPalettePresets(),
        [this]{ this->showPalettePresets(); }))
    ->add(
      new QuickCommand(
        CommandId::ShowPaletteOptions(),
        [this]{ this->showPaletteOptions(); }));
}

void ColorBar::showPaletteSortOptions()
{
  gfx::Rect bounds = m_buttons.getItem(
    static_cast<int>(PalButton::SORT))->bounds();

  Menu menu;
  MenuItem
    rev(Strings::color_bar_reverse_colors()),
    grd(Strings::color_bar_gradient()),
    grh(Strings::color_bar_gradient_by_hue()),
    hue(Strings::color_bar_sort_by_hue()),
    sat(Strings::color_bar_sort_by_saturation()),
    bri(Strings::color_bar_sort_by_brightness()),
    lum(Strings::color_bar_sort_by_luminance()),
    red(Strings::color_bar_sort_by_red()),
    grn(Strings::color_bar_sort_by_green()),
    blu(Strings::color_bar_sort_by_blue()),
    alp(Strings::color_bar_sort_by_alpha()),
    asc(Strings::color_bar_ascending()),
    des(Strings::color_bar_descending());
  menu.addChild(&rev);
  menu.addChild(&grd);
  menu.addChild(&grh);
  menu.addChild(new ui::MenuSeparator);
  menu.addChild(&hue);
  menu.addChild(&sat);
  menu.addChild(&bri);
  menu.addChild(&lum);
  menu.addChild(new ui::MenuSeparator);
  menu.addChild(&red);
  menu.addChild(&grn);
  menu.addChild(&blu);
  menu.addChild(&alp);
  menu.addChild(new ui::MenuSeparator);
  menu.addChild(&asc);
  menu.addChild(&des);

  if (m_ascending) asc.setSelected(true);
  else des.setSelected(true);

  rev.Click.connect([this]{ onReverseColors(); });
  grd.Click.connect([this]{ onGradient(GradientType::LINEAR); });
  grh.Click.connect([this]{ onGradient(GradientType::HUE); });
  hue.Click.connect([this]{ onSortBy(SortPaletteBy::HUE); });
  sat.Click.connect([this]{ onSortBy(SortPaletteBy::SATURATION); });
  bri.Click.connect([this]{ onSortBy(SortPaletteBy::VALUE); });
  lum.Click.connect([this]{ onSortBy(SortPaletteBy::LUMA); });
  red.Click.connect([this]{ onSortBy(SortPaletteBy::RED); });
  grn.Click.connect([this]{ onSortBy(SortPaletteBy::GREEN); });
  blu.Click.connect([this]{ onSortBy(SortPaletteBy::BLUE); });
  alp.Click.connect([this]{ onSortBy(SortPaletteBy::ALPHA); });
  asc.Click.connect([this]{ setAscending(true); });
  des.Click.connect([this]{ setAscending(false); });

  menu.showPopup(gfx::Point(bounds.x, bounds.y2()), display());
}

void ColorBar::showPalettePresets()
{
  if (!m_palettePopup) {
    try {
      m_palettePopup.reset(new PalettePopup());
    }
    catch (const std::exception& ex) {
      Console::showException(ex);
      return;
    }
  }

  if (!m_palettePopup->isVisible()) {
    gfx::Rect bounds = m_buttons.getItem(
      static_cast<int>(PalButton::PRESETS))->bounds();

    m_palettePopup->showPopup(display(), bounds);
  }
  else {
    m_palettePopup->closeWindow(nullptr);
  }
}

void ColorBar::showPaletteOptions()
{
  Menu* menu = AppMenus::instance()->getPalettePopupMenu();
  if (menu) {
    gfx::Rect bounds = m_buttons.getItem(
      static_cast<int>(PalButton::OPTIONS))->bounds();

    menu->showPopup(gfx::Point(bounds.x, bounds.y2()), display());
  }
}

bool ColorBar::canEditTiles() const
{
  const Site site = UIContext::instance()->activeSite();
  return (site.layer() &&
          site.layer()->isTilemap() &&
          !customTileManagement());
}

bool ColorBar::customTileManagement() const
{
  return (m_lastDocument &&
          m_lastDocument->sprite() &&
          m_lastDocument->sprite()->hasTileManagementPlugin());
}

} // namespace app
