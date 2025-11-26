// Aseprite
// Copyright (c) 2024-2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/font_entry.h"

#include "app/app.h"
#include "app/console.h"
#include "app/i18n/strings.h"
#include "app/recent_files.h"
#include "app/ui/font_popup.h"
#include "app/ui/skin/skin_theme.h"
#include "base/contains.h"
#include "base/convert_to.h"
#include "base/scoped_value.h"
#include "fmt/format.h"
#include "text/font_style_set.h"
#include "ui/display.h"
#include "ui/fit_bounds.h"
#include "ui/manager.h"
#include "ui/menu.h"
#include "ui/message.h"
#include "ui/scale.h"

#include "font_style.xml.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

namespace app {

using namespace ui;

FontEntry::FontFace::FontFace()
{
}

void FontEntry::FontFace::onInitTheme(InitThemeEvent& ev)
{
  SearchEntry::onInitTheme(ev);
  if (m_popup)
    m_popup->initTheme();
}

bool FontEntry::FontFace::onProcessMessage(Message* msg)
{
  switch (msg->type()) {
    // If we press the mouse button in the FontFace widget, and drag
    // the mouse (without releasing the mouse button) to the popup, we
    // send the mouse message to the popup.
    case kMouseMoveMessage:
      if (hasCapture() && m_popup) {
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
        const gfx::Point screenPos = mouseMsg->display()->nativeWindow()->pointToScreen(
          mouseMsg->position());
        Manager* mgr = manager();
        Widget* pick = mgr->pickFromScreenPos(screenPos);
        Widget* target = m_popup->getListBox();

        if (pick && (pick == target || pick->hasAncestor(target))) {
          mgr->transferAsMouseDownMessage(this, pick, mouseMsg);
          return true;
        }
      }
      break;

    case kMouseDownMessage:
    case kFocusEnterMessage:
      if (!isEnabled())
        break;

      if (!m_popup) {
        try {
          const FontInfo info = fontEntry()->info();

          m_popup.reset(new FontPopup(info));
          m_popup->FontChange.connect([this](const FontInfo& fontInfo) {
            FontChange(fontInfo,
                       m_fromEntryChange ? FontEntry::From::Face : FontEntry::From::Popup);
          });

          // If we press ESC in the popup we focus this FontFace field.
          m_popup->EscKey.connect([this]() { requestFocus(); });
        }
        catch (const std::exception& ex) {
          Console::showException(ex);
        }
      }
      if (!m_popup->isVisible()) {
        // Reset the search filter before opening the popup window.
        m_popup->setSearchText(std::string());

        m_popup->showPopup(display(), bounds());
        requestFocus();
      }
      break;

    case kFocusLeaveMessage: {
      // If we lost focus by a widget that is not part of the popup,
      // we close the popup.
      auto* newFocus = static_cast<FocusMessage*>(msg)->newFocus();
      if (m_popup && newFocus && newFocus->window() != m_popup.get()) {
        m_popup->closeWindow(nullptr);
      }

      // Restore the face name (e.g. when we press Escape key)
      const FontInfo info = fontEntry()->info();
      setText(info.title());
      break;
    }

    case kKeyDownMessage:
    case kKeyUpMessage:
      // If the popup is visible and we press the Up/Down arrow key,
      // we start navigating the popup list.
      if (hasFocus() && m_popup) {
        const auto* keymsg = static_cast<const KeyMessage*>(msg);
        switch (keymsg->scancode()) {
          case kKeyEsc:
          case kKeyEnter:
          case kKeyEnterPad:
            if (m_popup && m_popup->isVisible()) {
              m_popup->closeWindow(nullptr);

              // This final signal will release the focus from this
              // entry and give the chance to the client to focus
              // their own text box.
              FontChange(m_popup->selectedFont(), From::Popup);
              return true;
            }
            break;
          case kKeyUp:
          case kKeyDown:
            if (m_popup->isVisible()) {
              base::ScopedValue lock(fontEntry()->m_lockFace, true);

              // Redirect key message to the list box
              if (msg->recipient() == this) {
                // Redirect the Up/Down arrow key to the popup list
                // box, so we move through the list items. This will
                // not generate a FontChange (as it'd modify the
                // focused widget and other unexpected behaviors).
                m_popup->getListBox()->sendMessage(msg);

                // We are explicitly firing the FontChange signal so
                // the client knows the new selected font from the
                // popup.
                FontChange(m_popup->selectedFont(), From::Face);
              }
              return true;
            }
            break;
        }
        break;
      }
      break;
  }
  return SearchEntry::onProcessMessage(msg);
}

void FontEntry::FontFace::onChange()
{
  base::ScopedValue lock(m_fromEntryChange, true);
  SearchEntry::onChange();

  // This shouldn't happen, but we received crash reports where the
  // m_popup is nullptr here.
  ASSERT(m_popup);
  if (!m_popup)
    return;

  m_popup->setSearchText(text());

  // Changing the search text doesn't generate a FontChange
  // signal. Here we are forcing a FontChange signal with the first
  // selected font from the search. Indicating "From::Face" we avoid
  // changing the FontEntry text with the face font name (as the user
  // is writing the text to search, we don't want to touch this Entry
  // field).
  FontChange(m_popup->selectedFont(), From::Face);
}

os::Surface* FontEntry::FontFace::onGetCloseIcon() const
{
  auto& pinnedFonts = App::instance()->recentFiles()->pinnedFonts();
  const FontInfo info = fontEntry()->info();
  const std::string fontInfoStr = base::convert_to<std::string>(info);
  auto it = std::find(pinnedFonts.begin(), pinnedFonts.end(), fontInfoStr);
  if (it != pinnedFonts.end()) {
    return skin::SkinTheme::get(this)->parts.pinned()->bitmap(0);
  }
  return skin::SkinTheme::get(this)->parts.unpinned()->bitmap(0);
}

void FontEntry::FontFace::onCloseIconPressed()
{
  const FontInfo info = fontEntry()->info();
  if (info.size() == 0) // Don't save fonts with size=0pt
    return;

  auto& pinnedFonts = App::instance()->recentFiles()->pinnedFonts();
  const std::string fontInfoStr = base::convert_to<std::string>(info);

  auto it = std::find(pinnedFonts.begin(), pinnedFonts.end(), fontInfoStr);
  if (it != pinnedFonts.end()) {
    pinnedFonts.erase(it);
  }
  else {
    pinnedFonts.push_back(fontInfoStr);
    std::sort(pinnedFonts.begin(), pinnedFonts.end());
  }

  // This shouldn't happen, but we received crash reports where the
  // m_popup is nullptr here.
  ASSERT(m_popup);
  if (m_popup) {
    // Refill the list with the new pinned/unpinned item
    m_popup->recreatePinnedItems();
  }

  invalidate();
}

FontEntry::FontSize::FontSize()
{
  setEditable(true);
}

void FontEntry::FontSize::updateForFont(const FontInfo& info)
{
  std::vector<int> values = { 8, 9, 10, 11, 12, 14, 16, 18, 22, 24, 26, 28, 36, 48, 72 };
  int h = 0;

  // For SpriteSheet fonts we can offer the specific size that matches
  // the bitmap font (+ x2 + x3)
  text::FontRef font = Fonts::instance()->fontFromInfo(info);
  if (font && font->type() == text::FontType::SpriteSheet) {
    h = int(font->defaultSize());
    if (h > 0) {
      for (int i = h; i < h * 4; i += h) {
        if (!base::contains(values, i))
          values.insert(std::upper_bound(values.begin(), values.end(), i), i);
      }
    }
  }

  deleteAllItems();
  for (int i : values) {
    if (h && (i % h) == 0)
      addItem(fmt::format("{}*", i));
    else
      addItem(fmt::format("{}", i));
  }
}

void FontEntry::FontSize::onEntryChange()
{
  ComboBox::onEntryChange();
  Change();
}

FontEntry::FontStyle::FontStyle(ui::TooltipManager* tooltips) : ButtonSet(3, true)
{
  addItem("B");
  addItem("I");
  addItem("...");
  setMultiMode(MultiMode::Set);

  tooltips->addTooltipFor(getItem(0), Strings::font_style_font_weight(), BOTTOM);
  tooltips->addTooltipFor(getItem(1), Strings::text_tool_italic(), BOTTOM);
  tooltips->addTooltipFor(getItem(2), Strings::text_tool_more_options(), BOTTOM);
}

FontEntry::FontStroke::FontStroke(ui::TooltipManager* tooltips) : m_fill(2)
{
  auto* theme = skin::SkinTheme::get(this);

  m_fill.addItem(theme->parts.toolFilledRectangle(), theme->styles.contextBarButton());
  m_fill.addItem(theme->parts.toolRectangle(), theme->styles.contextBarButton());
  m_fill.setSelectedItem(0);
  m_fill.ItemChange.connect([this] { Change(); });

  m_stroke.setText("0");
  m_stroke.setSuffix("pt");
  m_stroke.ValueChange.connect([this] { Change(); });

  addChild(&m_fill);
  addChild(&m_stroke);

  tooltips->addTooltipFor(m_fill.getItem(0), Strings::shape_fill(), BOTTOM);
  tooltips->addTooltipFor(m_fill.getItem(1), Strings::shape_stroke(), BOTTOM);
  tooltips->addTooltipFor(&m_stroke, Strings::shape_stroke_width(), BOTTOM);
}

bool FontEntry::FontStroke::fill() const
{
  return const_cast<FontStroke*>(this)->m_fill.getItem(0)->isSelected();
}

float FontEntry::FontStroke::stroke() const
{
  return m_stroke.textDouble();
}

FontEntry::FontStroke::WidthEntry::WidthEntry() : ui::IntEntry(0, 100, this)
{
}

void FontEntry::FontStroke::WidthEntry::onValueChange()
{
  ui::IntEntry::onValueChange();
  ValueChange();
}

bool FontEntry::FontStroke::WidthEntry::onAcceptUnicodeChar(int unicodeChar)
{
  return (IntEntry::onAcceptUnicodeChar(unicodeChar) || unicodeChar == '.');
}

std::string FontEntry::FontStroke::WidthEntry::onGetTextFromValue(int value)
{
  return fmt::format("{:.1f}", value / 10.0);
}

int FontEntry::FontStroke::WidthEntry::onGetValueFromText(const std::string& text)
{
  return int(10.0 * base::convert_to<double>(text));
}

FontEntry::FontEntry(const bool withStrokeAndFill)
  : m_style(&m_tooltips)
  , m_stroke(withStrokeAndFill ? std::make_unique<FontStroke>(&m_tooltips) : nullptr)
{
  m_face.setExpansive(true);
  m_size.setExpansive(false);
  m_style.setExpansive(false);

  addChild(&m_tooltips);
  addChild(&m_face);
  addChild(&m_size);
  addChild(&m_style);
  if (m_stroke)
    addChild(m_stroke.get());

  m_tooltips.addTooltipFor(&m_face, Strings::text_tool_font_family(), BOTTOM);
  m_tooltips.addTooltipFor(m_size.getEntryWidget(), Strings::text_tool_font_size(), BOTTOM);

  m_face.setMinSize(gfx::Size(128 * guiscale(), 0));

  m_face.FontChange.connect([this](const FontInfo& newTypeName, const From from) {
    if (newTypeName.size() > 0)
      setInfo(newTypeName, from);
    else {
      setInfo(
        FontInfo(newTypeName, m_info.size(), m_info.style(), m_info.flags(), m_info.hinting()),
        from);
    }
    invalidate();
  });

  m_size.Change.connect([this]() {
    const float newSize = std::strtof(m_size.getValue().c_str(), nullptr);
    setInfo(FontInfo(m_info, newSize, m_info.style(), m_info.flags(), m_info.hinting()),
            From::Size);
  });

  m_style.ItemChange.connect(&FontEntry::onStyleItemClick, this);
  if (m_stroke)
    m_stroke->Change.connect(&FontEntry::onStrokeChange, this);
}

// Defined here as FontPopup type is not fully defined in the header
// file (and we have a std::unique_ptr<FontPopup> in FontEntry::FontFace).
FontEntry::~FontEntry()
{
}

void FontEntry::setInfo(const FontInfo& info, const From fromField)
{
  m_info = info;

  if (fromField == From::Init && !info.findTypeface(theme()->fontMgr())) {
    // Revert to default if we are initialized with an invalid/non-existent font
    m_info = skin::SkinTheme::get(this)->getDefaultFontInfo();
  }

  auto family = theme()->fontMgr()->matchFamily(m_info.name());
  bool hasBold = false;
  m_availableWeights.clear();

  if (family) {
    auto checkWeight = [this, &family, &hasBold](text::FontStyle::Weight w) {
      auto ref = family->matchStyle(
        text::FontStyle(w, m_info.style().width(), m_info.style().slant()));
      if (ref->fontStyle().weight() == w)
        m_availableWeights.push_back(w);

      if (ref->fontStyle().weight() == text::FontStyle::Weight::Bold)
        hasBold = true;
    };

    checkWeight(text::FontStyle::Weight::Thin);
    checkWeight(text::FontStyle::Weight::ExtraLight);
    checkWeight(text::FontStyle::Weight::Light);
    checkWeight(text::FontStyle::Weight::Normal);
    checkWeight(text::FontStyle::Weight::Medium);
    checkWeight(text::FontStyle::Weight::SemiBold);
    checkWeight(text::FontStyle::Weight::Bold);
    checkWeight(text::FontStyle::Weight::Black);
    checkWeight(text::FontStyle::Weight::ExtraBlack);
  }
  else {
    // Stick to only "normal" for fonts without a family.
    m_availableWeights.push_back(text::FontStyle::Weight::Normal);
    m_style.getItem(0)->setEnabled(false);
  }

  if (std::find(m_availableWeights.begin(), m_availableWeights.end(), m_info.style().weight()) ==
      m_availableWeights.end()) {
    // The currently selected weight is not available, reset it back to normal.
    m_info = app::FontInfo(m_info,
                           m_info.size(),
                           text::FontStyle(text::FontStyle::Weight::Normal,
                                           m_info.style().width(),
                                           m_info.style().slant()),
                           m_info.flags(),
                           m_info.hinting());
  }

  if (fromField != From::Face) {
    m_face.setText(m_info.title());
    m_face.setPlaceholder(m_info.title());
  }

  if (fromField != From::Size) {
    m_size.updateForFont(m_info);
    m_size.setValue(fmt::format("{}", m_info.size()));
  }

  m_style.getItem(0)->setEnabled(hasBold);
  m_style.getItem(0)->setSelected(m_info.style().weight() != text::FontStyle::Weight::Normal);
  m_style.getItem(0)->setText("B");

  // Give some indication of what the weight is, if we have any variation
  if (m_style.getItem(0)->isSelected() && m_availableWeights.size() > 1) {
    if (m_info.style().weight() > text::FontStyle::Weight::Bold)
      m_style.getItem(0)->setText("B+");
    else if (m_info.style().weight() < text::FontStyle::Weight::Bold)
      m_style.getItem(0)->setText("B-");
  }

  if (fromField != From::Style) {
    m_style.getItem(1)->setSelected(m_info.style().slant() != text::FontStyle::Slant::Upright);
  }

  FontChange(m_info, fromField);
}

ui::Paint FontEntry::paint()
{
  ui::Paint paint;
  ui::Paint::Style style = ui::Paint::Fill;

  if (m_stroke) {
    const float stroke = m_stroke->stroke();
    if (m_stroke->fill()) {
      if (stroke > 0.0f) {
        style = ui::Paint::StrokeAndFill;
        paint.strokeWidth(stroke);
      }
    }
    else {
      style = ui::Paint::Stroke;
      paint.strokeWidth(stroke);
    }
  }

  paint.style(style);
  return paint;
}

void FontEntry::onStyleItemClick(ButtonSet::Item* item)
{
  text::FontStyle style = m_info.style();

  switch (m_style.getItemIndex(item)) {
    // Bold button changed
    case 0: {
      if (m_availableWeights.size() > 2) {
        // Ensure consistency, since the click also affects the "selected" highlighting.
        item->setSelected(style.weight() != text::FontStyle::Weight::Normal);

        Menu weightMenu;
        auto currentWeight = m_info.style().weight();

        auto weightChange = [this](text::FontStyle::Weight newWeight) {
          const text::FontStyle style(newWeight, m_info.style().width(), m_info.style().slant());
          setInfo(FontInfo(m_info, m_info.size(), style, m_info.flags(), m_info.hinting()),
                  From::Style);
        };

        for (auto weight : m_availableWeights) {
          auto* menuItem = new MenuItem(Strings::Translate(
            fmt::format("font_style.font_weight_{}", static_cast<int>(weight)).c_str()));
          menuItem->setSelected(weight == currentWeight);
          if (!menuItem->isSelected())
            menuItem->Click.connect([&weightChange, weight] { weightChange(weight); });

          if (weight == text::FontStyle::Weight::Bold &&
              currentWeight == text::FontStyle::Weight::Normal)
            menuItem->setHighlighted(true);

          weightMenu.addChild(menuItem);
        }

        weightMenu.initTheme();
        const auto& bounds = m_style.getItem(0)->bounds();

        weightMenu.showPopup(gfx::Point(bounds.x, bounds.y2()), display());
      }
      else {
        const bool isBold = m_info.style().weight() == text::FontStyle::Weight::Bold;
        style = text::FontStyle(
          isBold ? text::FontStyle::Weight::Normal : text::FontStyle::Weight::Bold,
          style.width(),
          style.slant());

        setInfo(FontInfo(m_info, m_info.size(), style, m_info.flags(), m_info.hinting()),
                From::Style);
      }
      break;
    }
    // Italic button changed
    case 1: {
      const bool italic = m_style.getItem(1)->isSelected();
      style = text::FontStyle(
        style.weight(),
        style.width(),
        italic ? text::FontStyle::Slant::Italic : text::FontStyle::Slant::Upright);

      setInfo(FontInfo(m_info, m_info.size(), style, m_info.flags(), m_info.hinting()),
              From::Style);
      break;
    }
    case 2: {
      item->setSelected(false); // Unselect the "..." button

      ui::PopupWindow popup;
      app::gen::FontStyle content;

      content.antialias()->setSelected(m_info.antialias());
      content.ligatures()->setSelected(m_info.ligatures());
      content.hinting()->setSelected(m_info.hinting() == text::FontHinting::Normal);

      auto flagsChange = [this, &content]() {
        FontInfo::Flags flags = FontInfo::Flags::None;
        if (content.antialias()->isSelected())
          flags |= FontInfo::Flags::Antialias;
        if (content.ligatures()->isSelected())
          flags |= FontInfo::Flags::Ligatures;
        setInfo(FontInfo(m_info, m_info.size(), m_info.style(), flags, m_info.hinting()),
                From::Flags);
      };

      auto hintingChange = [this, &content]() {
        auto hinting = (content.hinting()->isSelected() ? text::FontHinting::Normal :
                                                          text::FontHinting::None);

        setInfo(FontInfo(m_info, m_info.size(), m_info.style(), m_info.flags(), hinting),
                From::Hinting);
      };

      content.antialias()->Click.connect(flagsChange);
      content.ligatures()->Click.connect(flagsChange);
      content.hinting()->Click.connect(hintingChange);

      popup.addChild(&content);
      popup.remapWindow();

      gfx::Rect rc = item->bounds();
      rc.y += rc.h - popup.border().bottom();

      ui::fit_bounds(display(), &popup, gfx::Rect(rc.origin(), popup.sizeHint()));

      popup.Open.connect([&popup] { popup.setHotRegion(gfx::Region(popup.boundsOnScreen())); });

      popup.openWindowInForeground();
      break;
    }
  }
}

void FontEntry::onStrokeChange()
{
  FontChange(m_info, From::Paint);
}

} // namespace app
