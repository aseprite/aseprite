// Aseprite
// Copyright (C) 2018-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#define COLOR_BAR_TRACE(...)

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/color_bar.h"

#include "app/app.h"
#include "app/app_menus.h"
#include "app/cmd/remap_colors.h"
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
#include "app/modules/editors.h"
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
#include "app/util/clipboard.h"
#include "base/bind.h"
#include "base/clamp.h"
#include "base/scoped_value.h"
#include "doc/cel.h"
#include "doc/cels_range.h"
#include "doc/image.h"
#include "doc/image_impl.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/remap.h"
#include "doc/rgbmap.h"
#include "doc/sort_palette.h"
#include "doc/sprite.h"
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

namespace app {

enum class PalButton {
  EDIT,
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
    setStyle(skin::SkinTheme::instance()->styles.warningBox());
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
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  setStyle(theme->styles.colorbarView());

  horizontalBar()->setStyle(theme->styles.miniScrollbar());
  verticalBar()->setStyle(theme->styles.miniScrollbar());
  horizontalBar()->setThumbStyle(theme->styles.miniScrollbarThumb());
  verticalBar()->setThumbStyle(theme->styles.miniScrollbarThumb());
}

//////////////////////////////////////////////////////////////////////
// ColorBar class

ColorBar* ColorBar::m_instance = NULL;

ColorBar::ColorBar(int align, TooltipManager* tooltipManager)
  : Box(align)
  , m_buttons(int(PalButton::MAX))
  , m_splitter(Splitter::ByPercentage, VERTICAL)
  , m_paletteView(true, PaletteView::FgBgColors, this, 16)
  , m_remapButton("Remap")
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
  , m_ascending(true)
  , m_lastButton(kButtonLeft)
  , m_editMode(false)
  , m_redrawTimer(250, this)
  , m_redrawAll(false)
  , m_implantChange(false)
  , m_selfPalChange(false)
{
  m_instance = this;

  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());

  m_buttons.addItem(theme->parts.timelineOpenPadlockActive());
  m_buttons.addItem(theme->parts.palSort());
  m_buttons.addItem(theme->parts.palPresets());
  m_buttons.addItem(theme->parts.palOptions());

  m_paletteView.setColumns(8);

  setup_mini_look(m_scrollableView.horizontalBar());
  setup_mini_look(m_scrollableView.verticalBar());

  m_scrollableView.attachToView(&m_paletteView);
  m_scrollableView.setExpansive(true);

  m_remapButton.setVisible(false);

  m_palettePlaceholder.addChild(&m_scrollableView);
  m_palettePlaceholder.addChild(&m_remapButton);
  m_splitter.setId("palette_spectrum_splitter");
  m_splitter.setPosition(80);
  m_splitter.setExpansive(true);
  m_splitter.addChild(&m_palettePlaceholder);
  m_splitter.addChild(&m_selectorPlaceholder);

  setColorSelector(
    Preferences::instance().colorBar.selector());

  addChild(&m_buttons);
  addChild(&m_splitter);

  HBox* fgBox = new HBox;
  HBox* bgBox = new HBox;
  fgBox->addChild(&m_fgColor);
  fgBox->addChild(m_fgWarningIcon);
  bgBox->addChild(&m_bgColor);
  bgBox->addChild(m_bgWarningIcon);
  addChild(fgBox);
  addChild(bgBox);

  m_fgColor.setId("fg_color");
  m_bgColor.setId("bg_color");
  m_fgColor.setExpansive(true);
  m_bgColor.setExpansive(true);

  m_remapButton.Click.connect(base::Bind<void>(&ColorBar::onRemapButtonClick, this));
  m_fgColor.Change.connect(&ColorBar::onFgColorButtonChange, this);
  m_fgColor.BeforeChange.connect(&ColorBar::onFgColorButtonBeforeChange, this);
  m_bgColor.Change.connect(&ColorBar::onBgColorButtonChange, this);
  m_fgWarningIcon->Click.connect(base::Bind<void>(&ColorBar::onFixWarningClick, this, &m_fgColor, m_fgWarningIcon));
  m_bgWarningIcon->Click.connect(base::Bind<void>(&ColorBar::onFixWarningClick, this, &m_bgColor, m_bgWarningIcon));
  m_redrawTimer.Tick.connect(base::Bind<void>(&ColorBar::onTimerTick, this));
  m_buttons.ItemChange.connect(base::Bind<void>(&ColorBar::onPaletteButtonClick, this));

  tooltipManager->addTooltipFor(&m_fgColor, "Foreground color", LEFT);
  tooltipManager->addTooltipFor(&m_bgColor, "Background color", LEFT);
  tooltipManager->addTooltipFor(m_fgWarningIcon, "Add foreground color to the palette", LEFT);
  tooltipManager->addTooltipFor(m_bgWarningIcon, "Add background color to the palette", LEFT);

  InitTheme.connect(
    [this, fgBox, bgBox]{
      SkinTheme* theme = static_cast<SkinTheme*>(this->theme());

      setBorder(gfx::Border(2*guiscale(), 0, 0, 0));
      setChildSpacing(2*guiscale());

      m_fgColor.resetSizeHint();
      m_bgColor.resetSizeHint();
      m_fgColor.setSizeHint(0, m_fgColor.sizeHint().h);
      m_bgColor.setSizeHint(0, m_bgColor.sizeHint().h);
      m_buttons.setMinSize(gfx::Size(0, theme->dimensions.colorBarButtonsHeight()));
      m_buttons.setMaxSize(gfx::Size(std::numeric_limits<int>::max(),
                                     std::numeric_limits<int>::max())); // TODO add resetMaxSize
      m_buttons.setMaxSize(gfx::Size(m_buttons.sizeHint().w,
                                     theme->dimensions.colorBarButtonsHeight()));

      int scrollBarWidth = theme->dimensions.miniScrollbarSize();
      m_scrollableView.horizontalBar()->setBarWidth(scrollBarWidth);
      m_scrollableView.verticalBar()->setBarWidth(scrollBarWidth);

      // Change color-bar background color (not ColorBar::setBgColor)
      this->Widget::setBgColor(theme->colors.tabActiveFace());
      m_paletteView.setBgColor(theme->colors.tabActiveFace());
      m_paletteView.setBoxSize(
        Preferences::instance().colorBar.boxSize());
      m_paletteView.initTheme();

      // Styles
      m_splitter.setStyle(theme->styles.workspaceSplitter());

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
  m_fgConn = Preferences::instance().colorBar.fgColor.AfterChange.connect(base::Bind<void>(&ColorBar::onFgColorChangeFromPreferences, this));
  m_bgConn = Preferences::instance().colorBar.bgColor.AfterChange.connect(base::Bind<void>(&ColorBar::onBgColorChangeFromPreferences, this));
  m_sepConn = Preferences::instance().colorBar.entriesSeparator.AfterChange.connect(base::Bind<void>(&ColorBar::invalidate, this));
  m_paletteView.FocusOrClick.connect(&ColorBar::onFocusPaletteView, this);
  m_appPalChangeConn = App::instance()->PaletteChange.connect(&ColorBar::onAppPaletteChange, this);
  KeyboardShortcuts::instance()->UserChange.connect(
    base::Bind<void>(&ColorBar::setupTooltips, this, tooltipManager));

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

PaletteView* ColorBar::getPaletteView()
{
  return &m_paletteView;
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
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  ButtonSet::Item* item = m_buttons.getItem((int)PalButton::EDIT);

  m_editMode = state;
  item->setIcon(state ? theme->parts.timelineOpenPadlockActive():
                        theme->parts.timelineClosedPadlockNormal());
  item->setHotColor(state ? theme->colors.editPalFace():
                            gfx::ColorNone);

  // Deselect color entries when we cancel editing
  if (!state)
    m_paletteView.deselect();
}

void ColorBar::onActiveSiteChange(const Site& site)
{
  if (m_lastDocument != site.document()) {
    if (m_lastDocument)
      m_lastDocument->remove_observer(this);

    m_lastDocument = const_cast<Doc*>(site.document());

    if (m_lastDocument)
      m_lastDocument->add_observer(this);

    hideRemap();
  }
}

void ColorBar::onGeneralUpdate(DocEvent& ev)
{
  // TODO Observe palette changes only
  invalidate();
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
  App::instance()->inputChain().prioritize(this, msg);
}

void ColorBar::onBeforeExecuteCommand(CommandExecutionEvent& ev)
{
  if (ev.command()->id() == CommandId::SetPalette() ||
      ev.command()->id() == CommandId::LoadPalette() ||
      ev.command()->id() == CommandId::ColorQuantization())
    showRemap();
}

void ColorBar::onAfterExecuteCommand(CommandExecutionEvent& ev)
{
  if (ev.command()->id() == CommandId::Undo() ||
      ev.command()->id() == CommandId::Redo())
    invalidate();

  // If the sprite isn't Indexed anymore (e.g. because we've just
  // undone a "RGB -> Indexed" conversion), we hide the "Remap"
  // button.
  Site site = UIContext::instance()->activeSite();
  if (site.sprite() &&
      site.sprite()->pixelFormat() != IMAGE_INDEXED) {
    hideRemap();
  }
}

// Switches the palette-editor
void ColorBar::onPaletteButtonClick()
{
  int item = m_buttons.selectedItem();
  m_buttons.deselectItems();

  switch (static_cast<PalButton>(item)) {

    case PalButton::EDIT:
      setEditMode(!inEditMode());
      break;

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

void ColorBar::onRemapButtonClick()
{
  ASSERT(m_oldPalette);

  // Create remap from m_oldPalette to the current palette
  Remap remap(1);
  try {
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
    ContextWriter writer(UIContext::instance());
    Sprite* sprite = writer.sprite();
    if (sprite) {
      ASSERT(sprite->pixelFormat() == IMAGE_INDEXED);

      Tx tx(writer.context(), "Remap Colors", ModifyDocument);
      bool remapPixels = true;

      if (remap.isFor8bit()) {
        PalettePicks usedEntries(256);

        for (const Cel* cel : sprite->uniqueCels()) {
          for (const auto& i : LockImageBits<IndexedTraits>(cel->image()))
            usedEntries[i] = true;
        }

        if (remap.isInvertible(usedEntries)) {
          tx(new cmd::RemapColors(sprite, remap));
          remapPixels = false;
        }
      }

      // Special remap saving original images in undo history
      if (remapPixels) {
        for (Cel* cel : sprite->uniqueCels()) {
          ImageRef celImage = cel->imageRef();
          ImageRef newImage(Image::createCopy(celImage.get()));
          doc::remap_image(newImage.get(), remap);

          tx(new cmd::ReplaceImage(
               sprite, celImage, newImage));
        }
      }

      color_t oldTransparent = sprite->transparentColor();
      color_t newTransparent = remap[oldTransparent];
      if (oldTransparent != newTransparent)
        tx(new cmd::SetTransparentColor(sprite, newTransparent));

      tx.commit();
    }
    update_screen_for_document(writer.document());
    hideRemap();
  }
  catch (base::Exception& e) {
    Console::showException(e);
  }
}

void ColorBar::onPaletteViewIndexChange(int index, ui::MouseButton button)
{
  COLOR_BAR_TRACE("ColorBar::onPaletteViewIndexChange(%d)\n", index);

  base::ScopedValue<bool> lock(m_fromPalView, true, m_fromPalView);

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
  showRemap();

  try {
    ContextWriter writer(UIContext::instance());
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

void ColorBar::onPaletteViewChangeSize(int boxsize)
{
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

void ColorBar::onFgColorChangeFromPreferences()
{
  COLOR_BAR_TRACE("ColorBar::onFgColorChangeFromPreferences() -> %s\n",
                  Preferences::instance().colorBar.fgColor().toString().c_str());

  if (m_fromPref)
    return;

  base::ScopedValue<bool> sync(m_fromPref, true, false);
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
    base::ScopedValue<bool> sync(m_fromPref, true, false);
    setBgColor(Preferences::instance().colorBar.bgColor());
  }
}

void ColorBar::onFgColorButtonBeforeChange(app::Color& color)
{
  COLOR_BAR_TRACE("ColorBar::onFgColorButtonBeforeChange(%s)\n", color.toString().c_str());

  if (m_fromPalView)
    return;

  if (!inEditMode()) {
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

  base::ScopedValue<bool> lock(m_fromFgButton, true, false);

  if (!m_fromPref) {
    base::ScopedValue<bool> sync(m_fromPref, true, false);
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

  base::ScopedValue<bool> lock(m_fromBgButton, true, false);

  if (!m_fromPalView && !inEditMode())
    m_paletteView.deselect();

  if (!m_fromPref) {
    base::ScopedValue<bool> sync(m_fromPref, true, false);
    Preferences::instance().colorBar.bgColor(color);
  }

  updateWarningIcon(color, m_bgWarningIcon);
  onColorButtonChange(color);
}

void ColorBar::onColorButtonChange(const app::Color& color)
{
  COLOR_BAR_TRACE("ColorBar::onColorButtonChange(%s)\n", color.toString().c_str());

  if (!inEditMode() ||
      m_fromPref) {
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

void ColorBar::onPickSpectrum(const app::Color& color, ui::MouseButton button)
{
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
  setPalette(&newPalette, "Reverse Colors");
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
  setPalette(&newPalette, "Sort Colors");
}

void ColorBar::onGradient()
{
  int index1, index2;
  if (!m_paletteView.getSelectedRange(index1, index2))
    return;

  Palette newPalette(*get_current_palette());
  newPalette.makeGradient(index1, index2);

  setPalette(&newPalette, "Gradient");
}

void ColorBar::setAscending(bool ascending)
{
  m_ascending = ascending;
}

void ColorBar::showRemap()
{
  Site site = UIContext::instance()->activeSite();
  if (site.sprite() &&
      site.sprite()->pixelFormat() == IMAGE_INDEXED) {
    if (!m_oldPalette) {
      m_oldPalette.reset(new Palette(*get_current_palette()));
      m_remapButton.setVisible(true);
      layout();
    }
  }
}

void ColorBar::hideRemap()
{
  if (!m_oldPalette)
    return;

  m_oldPalette.reset();
  m_remapButton.setVisible(false);
  layout();
}

void ColorBar::onNewInputPriority(InputChainElement* element,
                                  const ui::Message* msg)
{
  if (dynamic_cast<Timeline*>(element) &&
      msg && (msg->ctrlPressed() || msg->shiftPressed()))
    return;

  if (element != this)
    m_paletteView.deselect();
}

bool ColorBar::onCanCut(Context* ctx)
{
  return (m_paletteView.getSelectedEntriesCount() > 0);
}

bool ColorBar::onCanCopy(Context* ctx)
{
  return (m_paletteView.getSelectedEntriesCount() > 0);
}

bool ColorBar::onCanPaste(Context* ctx)
{
  return (clipboard::get_current_format() == clipboard::ClipboardPaletteEntries);
}

bool ColorBar::onCanClear(Context* ctx)
{
  return (m_paletteView.getSelectedEntriesCount() > 0);
}

bool ColorBar::onCut(Context* ctx)
{
  m_paletteView.cutToClipboard();
  return true;
}

bool ColorBar::onCopy(Context* ctx)
{
  m_paletteView.copyToClipboard();
  return true;
}

bool ColorBar::onPaste(Context* ctx)
{
  m_paletteView.pasteFromClipboard();
  return true;
}

bool ColorBar::onClear(Context* ctx)
{
  m_paletteView.clearSelection();
  return true;
}

void ColorBar::onCancel(Context* ctx)
{
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
      base::ScopedValue<bool> sync(m_fromPref, true, m_fromPref);
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
    if (current_editor)
      current_editor->updateEditor(true);
  }
}

void ColorBar::updateWarningIcon(const app::Color& color, ui::Button* warningIcon)
{
  int index = -1;

  if (color.getType() == app::Color::MaskType) {
    if (current_editor &&
        current_editor->sprite()) {
      index = current_editor->sprite()->transparentColor();
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
        Cmd* cmd = new cmd::SetPalette(sprite, frame, newPalette);

        // Add undo information to save the range of pal entries that will be modified.
        if (m_implantChange &&
            undo->lastExecutedCmd() &&
            undo->lastExecutedCmd()->label() == operationName) {
          // Implant the cmd in the last CmdSequence if it's
          // related about color palette modifications
          ASSERT(dynamic_cast<CmdSequence*>(undo->lastExecutedCmd()));
          static_cast<CmdSequence*>(undo->lastExecutedCmd())->add(cmd);
          cmd->execute(UIContext::instance());
        }
        else {
          Tx tx(writer.context(), operationName, ModifyDocument);
          tx(cmd);
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
  tooltipManager->addTooltipFor(
    m_buttons.getItem((int)PalButton::EDIT),
    key_tooltip("Edit Color", CommandId::PaletteEditor()), BOTTOM);

  tooltipManager->addTooltipFor(m_buttons.getItem((int)PalButton::SORT), "Sort & Gradients", BOTTOM);
  tooltipManager->addTooltipFor(m_buttons.getItem((int)PalButton::PRESETS), "Presets", BOTTOM);
  tooltipManager->addTooltipFor(m_buttons.getItem((int)PalButton::OPTIONS), "Options", BOTTOM);
  tooltipManager->addTooltipFor(&m_remapButton, "Matches old indexes with new indexes", BOTTOM);
}

// static
void ColorBar::fixColorIndex(ColorButton& colorButton)
{
  app::Color color = colorButton.getColor();

  if (color.getType() == Color::IndexType) {
    int oldIndex = color.getIndex();
    int newIndex = base::clamp(oldIndex, 0, get_current_palette()->size()-1);
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
    rev("Reverse Colors"),
    grd("Gradient"),
    hue("Sort by Hue"),
    sat("Sort by Saturation"),
    bri("Sort by Brightness"),
    lum("Sort by Luminance"),
    red("Sort by Red"),
    grn("Sort by Green"),
    blu("Sort by Blue"),
    alp("Sort by Alpha"),
    asc("Ascending"),
    des("Descending");
  menu.addChild(&rev);
  menu.addChild(&grd);
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

  rev.Click.connect(base::Bind<void>(&ColorBar::onReverseColors, this));
  grd.Click.connect(base::Bind<void>(&ColorBar::onGradient, this));
  hue.Click.connect(base::Bind<void>(&ColorBar::onSortBy, this, SortPaletteBy::HUE));
  sat.Click.connect(base::Bind<void>(&ColorBar::onSortBy, this, SortPaletteBy::SATURATION));
  bri.Click.connect(base::Bind<void>(&ColorBar::onSortBy, this, SortPaletteBy::VALUE));
  lum.Click.connect(base::Bind<void>(&ColorBar::onSortBy, this, SortPaletteBy::LUMA));
  red.Click.connect(base::Bind<void>(&ColorBar::onSortBy, this, SortPaletteBy::RED));
  grn.Click.connect(base::Bind<void>(&ColorBar::onSortBy, this, SortPaletteBy::GREEN));
  blu.Click.connect(base::Bind<void>(&ColorBar::onSortBy, this, SortPaletteBy::BLUE));
  alp.Click.connect(base::Bind<void>(&ColorBar::onSortBy, this, SortPaletteBy::ALPHA));
  asc.Click.connect(base::Bind<void>(&ColorBar::setAscending, this, true));
  des.Click.connect(base::Bind<void>(&ColorBar::setAscending, this, false));

  menu.showPopup(gfx::Point(bounds.x, bounds.y+bounds.h));
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

    m_palettePopup->showPopup(
      gfx::Rect(bounds.x, bounds.y+bounds.h,
                ui::display_w()/2, ui::display_h()*3/4));
  }
  else {
    m_palettePopup->closeWindow(NULL);
  }
}

void ColorBar::showPaletteOptions()
{
  Menu* menu = AppMenus::instance()->getPalettePopupMenu();
  if (menu) {
    gfx::Rect bounds = m_buttons.getItem(
      static_cast<int>(PalButton::OPTIONS))->bounds();

    menu->showPopup(gfx::Point(bounds.x, bounds.y+bounds.h));
  }
}

} // namespace app
