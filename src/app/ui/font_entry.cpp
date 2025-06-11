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
#include "app/recent_files.h"
#include "app/ui/font_popup.h"
#include "app/ui/skin/skin_theme.h"
#include "base/contains.h"
#include "base/scoped_value.h"
#include "fmt/format.h"
#include "ui/display.h"
#include "ui/fit_bounds.h"
#include "ui/manager.h"
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

FontEntry::FontStyle::FontStyle() : ButtonSet(3, true)
{
  addItem("B");
  addItem("I");
  addItem("...");
  setMultiMode(MultiMode::Set);
}

FontEntry::FontEntry()
{
  m_face.setExpansive(true);
  m_size.setExpansive(false);
  m_style.setExpansive(false);
  addChild(&m_face);
  addChild(&m_size);
  addChild(&m_style);

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
}

// Defined here as FontPopup type is not fully defined in the header
// file (and we have a std::unique_ptr<FontPopup> in FontEntry::FontFace).
FontEntry::~FontEntry()
{
}

void FontEntry::setInfo(const FontInfo& info, const From fromField)
{
  m_info = info;

  if (fromField != From::Face)
    m_face.setText(info.title());

  if (fromField != From::Size) {
    m_size.updateForFont(info);
    m_size.setValue(fmt::format("{}", info.size()));
  }

  if (fromField != From::Style) {
    m_style.getItem(0)->setSelected(info.style().weight() >= text::FontStyle::Weight::SemiBold);
    m_style.getItem(1)->setSelected(info.style().slant() != text::FontStyle::Slant::Upright);
  }

  FontChange(m_info, fromField);
}

void FontEntry::onStyleItemClick(ButtonSet::Item* item)
{
  text::FontStyle style = m_info.style();

  switch (m_style.getItemIndex(item)) {
    // Bold button changed
    case 0: {
      const bool bold = m_style.getItem(0)->isSelected();
      style = text::FontStyle(
        bold ? text::FontStyle::Weight::Bold : text::FontStyle::Weight::Normal,
        style.width(),
        style.slant());

      setInfo(FontInfo(m_info, m_info.size(), style, m_info.flags(), m_info.hinting()),
              From::Style);
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

} // namespace app
