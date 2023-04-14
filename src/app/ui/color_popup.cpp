// Aseprite
// Copyright (C) 2020-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
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
#include "app/doc.h"
#include "app/file/palette_file.h"
#include "app/i18n/strings.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/resource_finder.h"
#include "app/transaction.h"
#include "app/ui/color_bar.h"
#include "app/ui/palette_view.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui_context.h"
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
  HSV_MODE,
  HSL_MODE,
  GRAY_MODE,
  MASK_MODE,
  COLOR_MODES
};

static std::unique_ptr<doc::Palette> g_simplePal(nullptr);

class ColorPopup::SimpleColors : public HBox {
public:

  class Item : public Button {
  public:
    Item(ColorPopup* colorPopup, const app::Color& color)
      : Button("")
      , m_colorPopup(colorPopup)
      , m_color(color) {
    }

  private:
    void onClick() override {
      m_colorPopup->setColorWithSignal(m_color, ChangeType);
    }

    void onPaint(PaintEvent& ev) override {
      Graphics* g = ev.graphics();
      auto theme = skin::SkinTheme::get(this);
      gfx::Rect rc = clientBounds();

      Button::onPaint(ev);

      rc.shrink(theme->calcBorder(this, style()));
      draw_color(g, rc, m_color, doc::ColorMode::RGB);
    }

    ColorPopup* m_colorPopup;
    app::Color m_color;
  };

  SimpleColors(ColorPopup* colorPopup, TooltipManager* tooltips) {
    for (int i=0; i<g_simplePal->size(); ++i) {
      doc::color_t c = g_simplePal->getEntry(i);
      app::Color color =
        app::Color::fromRgb(doc::rgba_getr(c),
                            doc::rgba_getg(c),
                            doc::rgba_getb(c),
                            doc::rgba_geta(c));

      Item* item = new Item(colorPopup, color);
      item->InitTheme.connect(
        [item]{
          auto theme = skin::SkinTheme::get(item);
          item->setSizeHint(gfx::Size(16, 16)*ui::guiscale());
          item->setStyle(theme->styles.simpleColor());
        });
      item->initTheme();
      addChild(item);

      tooltips->addTooltipFor(
        item, g_simplePal->getEntryName(i), BOTTOM);
    }
  }

  void selectColor(int index) {
    for (int i=0; i<g_simplePal->size(); ++i) {
      children()[i]->setSelected(i == index);
    }
  }

  void deselect() {
    for (int i=0; i<g_simplePal->size(); ++i) {
      children()[i]->setSelected(false);
    }
  }
};

ColorPopup::CustomButtonSet::CustomButtonSet()
  : ButtonSet(COLOR_MODES)
{
}

void ColorPopup::CustomButtonSet::onSelectItem(Item* item, bool focusItem, ui::Message* msg)
{
  int count = countSelectedItems();
  int itemIndex = getItemIndex(item);

  if (itemIndex == INDEX_MODE ||
      itemIndex == MASK_MODE ||
      !msg ||
      // Any key modifier will act as multiple selection
      (!msg->shiftPressed() &&
       !msg->altPressed() &&
       !msg->ctrlPressed() &&
       !msg->cmdPressed())) {
    if (item &&
        item->isSelected() &&
        count == 1)
      return;

    for (int i=0; i<COLOR_MODES; ++i)
      if (getItem(i)->isSelected())
        getItem(i)->setSelected(false);
  }
  else {
    if (getItem(INDEX_MODE)->isSelected()) getItem(INDEX_MODE)->setSelected(false);
    if (getItem(MASK_MODE)->isSelected()) getItem(MASK_MODE)->setSelected(false);
  }

  if (item) {
    // Item already selected
    if (count == 1 && item == findSelectedItem())
      return;

    item->setSelected(!item->isSelected());
    if (focusItem)
      item->requestFocus();
  }
}

ColorPopup::ColorPopup(const ColorButtonOptions& options)
  : PopupWindowPin(" ", // Non-empty to create title-bar and close button
                   ClickBehavior::CloseOnClickInOtherWindow,
                   options.canPinSelector)
  , m_vbox(VERTICAL)
  , m_topBox(HORIZONTAL)
  , m_color(app::Color::fromMask())
  , m_closeButton(nullptr)
  , m_colorPaletteContainer(options.showIndexTab ?
                            new ui::View: nullptr)
  , m_colorPalette(options.showIndexTab ?
                   new PaletteView(false, PaletteView::SelectOneColor, this, 7*guiscale()):
                   nullptr)
  , m_simpleColors(nullptr)
  , m_oldAndNew(Shade(2), ColorShades::ClickEntries)
  , m_maskLabel(Strings::color_popup_transparent_color_sel())
  , m_canPin(options.canPinSelector)
  , m_insideChange(false)
  , m_disableHexUpdate(false)
{
  if (options.showSimpleColors) {
    if (!g_simplePal) {
      ResourceFinder rf;
      rf.includeDataDir("palettes/tags.gpl");
      if (rf.findFirst())
        g_simplePal = load_palette(rf.filename().c_str());
    }

    if (g_simplePal)
      m_simpleColors = new SimpleColors(this, &m_tooltips);
  }

  ButtonSet::Item* item = m_colorType.addItem(Strings::color_popup_index());
  item->setFocusStop(false);
  if (!options.showIndexTab)
    item->setVisible(false);

  m_colorType.addItem("RGB")->setFocusStop(false);
  m_colorType.addItem("HSV")->setFocusStop(false);
  m_colorType.addItem("HSL")->setFocusStop(false);
  m_colorType.addItem("Gray")->setFocusStop(false);
  m_colorType.addItem("Mask")->setFocusStop(false);

  m_topBox.setBorder(gfx::Border(0));
  m_topBox.setChildSpacing(0);

  if (m_colorPalette) {
    m_colorPaletteContainer->attachToView(m_colorPalette);
    m_colorPaletteContainer->setExpansive(true);
  }
  m_sliders.setExpansive(true);

  m_topBox.addChild(&m_colorType);
  m_topBox.addChild(new Separator("", VERTICAL));
  m_topBox.addChild(&m_hexColorEntry);
  m_topBox.addChild(&m_oldAndNew);

  // TODO fix this hack for close button in popup window
  // Move close button (decorative widget) inside the m_topBox
  {
    WidgetsList decorators;
    for (auto child : children()) {
      if (child->type() == kWindowCloseButtonWidget) {
        m_closeButton = child;
        removeChild(child);
        break;
      }
    }
    if (m_closeButton) {
      m_topBox.addChild(new BoxFiller);
      VBox* vbox = new VBox;
      vbox->addChild(m_closeButton);
      m_topBox.addChild(vbox);
    }
  }
  setText("");                  // To remove title

  m_vbox.addChild(&m_tooltips);
  if (m_simpleColors)
    m_vbox.addChild(m_simpleColors);
  m_vbox.addChild(&m_topBox);
  if (m_colorPaletteContainer)
    m_vbox.addChild(m_colorPaletteContainer);
  m_vbox.addChild(&m_sliders);
  m_vbox.addChild(&m_maskLabel);
  addChild(&m_vbox);

  m_colorType.ItemChange.connect([this]{ onColorTypeClick(); });

  m_sliders.ColorChange.connect(&ColorPopup::onColorSlidersChange, this);
  m_hexColorEntry.ColorChange.connect(&ColorPopup::onColorHexEntryChange, this);
  m_oldAndNew.Click.connect(&ColorPopup::onSelectOldColor, this);

  // Set RGB just for the sizeHint(), and then deselect the color type
  // (the first setColor() call will setup it correctly.)
  selectColorType(app::Color::RgbType);
  m_colorType.deselectItems();

  m_onPaletteChangeConn =
    App::instance()->PaletteChange.connect(&ColorPopup::onPaletteChange, this);

  InitTheme.connect(
    [this]{
      setSizeHint(gfx::Size(300*guiscale(), sizeHint().h));
    });
  initTheme();
}

ColorPopup::~ColorPopup()
{
}

void ColorPopup::setColor(const app::Color& color,
                          const SetColorOptions options)
{
  m_color = color;

  if (m_simpleColors) {
    int r = color.getRed();
    int g = color.getGreen();
    int b = color.getBlue();
    int a = color.getAlpha();
    int i = g_simplePal->findExactMatch(r, g, b, a, -1);
    if (i >= 0)
      m_simpleColors->selectColor(i);
    else
      m_simpleColors->deselect();
  }

  if (color.getType() == app::Color::IndexType) {
    if (m_colorPalette) {
      m_colorPalette->deselect();
      m_colorPalette->selectColor(color.getIndex());
    }
  }

  m_sliders.setColor(m_color);
  if (!m_disableHexUpdate)
    m_hexColorEntry.setColor(m_color);

  if (options == ChangeType)
    selectColorType(m_color.getType());

  // Set the new color
  Shade shade = m_oldAndNew.getShade();
  shade.resize(2);
  shade[1] = (color.getType() == app::Color::IndexType ? color.toRgb(): color);
  if (!m_insideChange)
    shade[0] = shade[1];
  m_oldAndNew.setShade(shade);
}

app::Color ColorPopup::getColor() const
{
  return m_color;
}

bool ColorPopup::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {
    case kSetCursorMessage: {
      if (m_canPin) {
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
        if (hitTest(mousePos) == HitTestCaption) {
          set_mouse_cursor(kMoveCursor);
          return true;
        }
      }
      break;
    }
  }
  return PopupWindowPin::onProcessMessage(msg);
}

void ColorPopup::onWindowResize()
{
  PopupWindowPin::onWindowResize();

  if (m_closeButton) {
    gfx::Rect rc = m_closeButton->bounds();
    if (rc.x2() > bounds().x2()) {
      rc.x = bounds().x2() - rc.w;
      m_closeButton->setBounds(rc);
    }
  }
}

void ColorPopup::onMakeFloating()
{
  PopupWindowPin::onMakeFloating();

  if (m_canPin) {
    setSizeable(true);
    setMoveable(true);
  }
}

void ColorPopup::onMakeFixed()
{
  PopupWindowPin::onMakeFixed();

  if (m_canPin) {
    setSizeable(false);
    setMoveable(true);
  }
}

void ColorPopup::onPaletteViewIndexChange(int index, ui::MouseButton button)
{
  base::ScopedValue restore(m_insideChange, true);

  setColorWithSignal(app::Color::fromIndex(index), ChangeType);
}

void ColorPopup::onColorSlidersChange(ColorSlidersChangeEvent& ev)
{
  base::ScopedValue restore(m_insideChange, true);

  setColorWithSignal(ev.color(), DontChangeType);
  findBestfitIndex(ev.color());
}

void ColorPopup::onColorHexEntryChange(const app::Color& color)
{
  base::ScopedValue restore(m_insideChange, true);

  // Disable updating the hex entry so we don't override what the user
  // is writting in the text field.
  m_disableHexUpdate = true;

  setColorWithSignal(color, ChangeType);
  findBestfitIndex(color);

  // If we are in edit mode, the "m_disableHexUpdate" will be changed
  // to false in onPaletteChange() after the color bar timer is
  // triggered. In this way we don't modify the hex field when the
  // user is editing it and the palette "edit mode" is enabled.
  if (!inEditMode())
    m_disableHexUpdate = false;
}

void ColorPopup::onSelectOldColor(ColorShades::ClickEvent&)
{
  Shade shade = m_oldAndNew.getShade();
  int hot = m_oldAndNew.getHotEntry();
  if (hot >= 0 &&
      hot < int(shade.size())) {
    setColorWithSignal(shade[hot], DontChangeType);
  }
}

void ColorPopup::onSimpleColorClick()
{
  m_colorType.deselectItems();
  if (!g_simplePal)
    return;

  app::Color color = getColor();

  // Find bestfit palette entry
  int r = color.getRed();
  int g = color.getGreen();
  int b = color.getBlue();
  int a = color.getAlpha();

  // Search for the closest color to the RGB values
  int i = g_simplePal->findBestfit(r, g, b, a, 0);
  if (i >= 0) {
    color_t c = g_simplePal->getEntry(i);
    color = app::Color::fromRgb(doc::rgba_getr(c),
                                doc::rgba_getg(c),
                                doc::rgba_getb(c),
                                doc::rgba_geta(c));
  }

  setColorWithSignal(color, ChangeType);
}

void ColorPopup::onColorTypeClick()
{
  base::ScopedValue restore(m_insideChange, true);

  if (m_simpleColors)
    m_simpleColors->deselect();

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
    case HSV_MODE:
      newColor = app::Color::fromHsv(newColor.getHsvHue(),
                                     newColor.getHsvSaturation(),
                                     newColor.getHsvValue(),
                                     newColor.getAlpha());
      break;
    case HSL_MODE:
      newColor = app::Color::fromHsl(newColor.getHslHue(),
                                     newColor.getHslSaturation(),
                                     newColor.getHslLightness(),
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

  setColorWithSignal(newColor, ChangeType);
}

void ColorPopup::onPaletteChange()
{
  base::ScopedValue restore(m_insideChange, inEditMode());

  setColor(getColor(), DontChangeType);
  invalidate();

  if (inEditMode())
    m_disableHexUpdate = false;
}

void ColorPopup::findBestfitIndex(const app::Color& color)
{
  if (!m_colorPalette)
    return;

  // Find bestfit palette entry
  int r = color.getRed();
  int g = color.getGreen();
  int b = color.getBlue();
  int a = color.getAlpha();

  // Search for the closest color to the RGB values
  int i = get_current_palette()->findBestfit(r, g, b, a, 0);
  if (i >= 0) {
    m_colorPalette->deselect();
    m_colorPalette->selectColor(i);
  }
}

void ColorPopup::setColorWithSignal(const app::Color& color,
                                    const SetColorOptions options)
{
  Shade shade = m_oldAndNew.getShade();

  setColor(color, options);

  shade.resize(2);
  shade[1] = color;
  m_oldAndNew.setShade(shade);

  // Fire ColorChange signal
  ColorChange(color);
}

void ColorPopup::selectColorType(app::Color::Type type)
{
  if (m_colorPaletteContainer)
    m_colorPaletteContainer->setVisible(type == app::Color::IndexType);

  m_maskLabel.setVisible(type == app::Color::MaskType);

  // Count selected items.
  if (m_colorType.countSelectedItems() < 2) {
    switch (type) {
      case app::Color::IndexType:
        if (m_colorPalette)
          m_colorType.setSelectedItem(INDEX_MODE);
        else
          m_colorType.setSelectedItem(RGB_MODE);
        break;
      case app::Color::RgbType:   m_colorType.setSelectedItem(RGB_MODE); break;
      case app::Color::HsvType:   m_colorType.setSelectedItem(HSV_MODE); break;
      case app::Color::HslType:   m_colorType.setSelectedItem(HSL_MODE); break;
      case app::Color::GrayType:  m_colorType.setSelectedItem(GRAY_MODE); break;
      case app::Color::MaskType:  m_colorType.setSelectedItem(MASK_MODE); break;
    }
  }

  std::vector<app::Color::Type> types;
  if (m_colorType.getItem(RGB_MODE)->isSelected())
    types.push_back(app::Color::RgbType);
  if (m_colorType.getItem(HSV_MODE)->isSelected())
    types.push_back(app::Color::HsvType);
  if (m_colorType.getItem(HSL_MODE)->isSelected())
    types.push_back(app::Color::HslType);
  if (m_colorType.getItem(GRAY_MODE)->isSelected())
    types.push_back(app::Color::GrayType);
  m_sliders.setColorTypes(types);

  // Remove focus from hidden RGB/HSV/HSL text entries
  auto widget = manager()->getFocus();
  if (widget && !widget->isVisible()) {
    auto window = widget->window();
    if (window && window == this)
      widget->releaseFocus();
  }

  m_vbox.layout();
  m_vbox.invalidate();
}

bool ColorPopup::inEditMode()
{
  return
    // TODO use other flag instead of m_canPin, here we want to ask if
    // this ColorPopup is related to the main ColorBar (instead of
    // other ColorButtons like the one in "Replace Color", etc.)
    (m_canPin) &&
    (ColorBar::instance()->inEditMode());
}

} // namespace app
