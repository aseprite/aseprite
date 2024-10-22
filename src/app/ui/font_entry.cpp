// Aseprite
// Copyright (c) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/font_entry.h"

#include "app/console.h"
#include "app/ui/font_popup.h"
#include "base/scoped_value.h"
#include "fmt/format.h"
#include "ui/display.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/scale.h"

#include <cstdlib>

namespace app {

using namespace ui;

FontEntry::FontFace::FontFace()
{
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
        const gfx::Point screenPos =
          mouseMsg->display()->nativeWindow()->pointToScreen(mouseMsg->position());
        Widget* pick = manager()->pickFromScreenPos(screenPos);
        Widget* target = m_popup->getListBox();

        if (pick && (pick == target ||
                     pick->hasAncestor(target))) {
          releaseMouse();

          MouseMessage mouseMsg2(
            kMouseDownMessage,
            *mouseMsg,
            mouseMsg->positionForDisplay(pick->display()));
          mouseMsg2.setRecipient(pick);
          mouseMsg2.setDisplay(pick->display());
          pick->sendMessage(&mouseMsg2);
          return true;
        }
      }
      break;

    case kMouseDownMessage:
    case kFocusEnterMessage:
      if (!m_popup) {
        try {
          const FontInfo info = fontEntry()->info();

          m_popup.reset(new FontPopup(info));
          m_popup->FontChange.connect([this](const FontInfo& fontInfo){
            FontChange(fontInfo,
                       m_fromEntryChange ?
                       FontEntry::From::Face:
                       FontEntry::From::Popup);
          });

          // If we press ESC in the popup we focus this FontFace field.
          m_popup->EscKey.connect([this](){
            requestFocus();
          });
        }
        catch (const std::exception& ex) {
          Console::showException(ex);
        }
      }
      if (!m_popup->isVisible()) {
        m_popup->showPopup(display(), bounds());
        requestFocus();
      }
      break;

    case kFocusLeaveMessage: {
      // If we lost focus by a widget that is not part of the popup,
      // we close the popup.
      auto* newFocus = static_cast<FocusMessage*>(msg)->newFocus();
      if (m_popup &&
          newFocus &&
          newFocus->window() != m_popup.get()) {
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

  m_popup->setSearchText(text());

  // Changing the search text doesn't generate a FontChange
  // signal. Here we are forcing a FontChange signal with the first
  // selected font from the search. Indicating "From::Face" we avoid
  // changing the FontEntry text with the face font name (as the user
  // is writing the text to search, we don't want to touch this Entry
  // field).
  FontChange(m_popup->selectedFont(), From::Face);
}

FontEntry::FontSize::FontSize()
{
  setEditable(true);
  for (int i : { 8, 9, 10, 11, 12, 14, 16, 18, 22, 24, 26, 28, 36, 48, 72 })
    addItem(fmt::format("{}", i));
}

void FontEntry::FontSize::onEntryChange()
{
  ComboBox::onEntryChange();
  Change();
}

FontEntry::FontStyle::FontStyle()
  : ButtonSet(2, true)
{
  addItem("B");
  addItem("I");
  setMultiMode(MultiMode::Set);
}

FontEntry::FontLigatures::FontLigatures()
  : ButtonSet(1, true)
{
  addItem("fi");
  setMultiMode(MultiMode::Set);
}

FontEntry::FontEntry()
  : m_antialias("Antialias")
{
  m_face.setExpansive(true);
  m_size.setExpansive(false);
  m_style.setExpansive(false);
  m_ligatures.setExpansive(false);
  m_antialias.setExpansive(false);
  addChild(&m_face);
  addChild(&m_size);
  addChild(&m_style);
  addChild(&m_ligatures);
  addChild(&m_antialias);

  m_face.setMinSize(gfx::Size(128*guiscale(), 0));

  m_face.FontChange.connect([this](const FontInfo& newTypeName,
                                   const From from) {
    setInfo(FontInfo(newTypeName,
                     m_info.size(),
                     m_info.style(),
                     m_info.flags()),
            from);
    invalidate();
  });

  m_size.Change.connect([this](){
    const float newSize = std::strtof(m_size.getValue().c_str(), nullptr);
    setInfo(FontInfo(m_info,
                     newSize,
                     m_info.style(),
                     m_info.flags()),
            From::Size);
  });

  m_style.ItemChange.connect([this](ButtonSet::Item* item){
    text::FontStyle style = m_info.style();
    switch (m_style.getItemIndex(item)) {
      // Bold button changed
      case 0: {
        const bool bold = m_style.getItem(0)->isSelected();
        style = text::FontStyle(bold ? text::FontStyle::Weight::Bold:
                                       text::FontStyle::Weight::Normal,
                                style.width(),
                                style.slant());
        break;
      }
      // Italic button changed
      case 1: {
        const bool italic = m_style.getItem(1)->isSelected();
        style = text::FontStyle(style.weight(),
                                style.width(),
                                italic ? text::FontStyle::Slant::Italic:
                                         text::FontStyle::Slant::Upright);
        break;
      }
    }

    setInfo(FontInfo(m_info,
                     m_info.size(),
                     style,
                     m_info.flags()),
            From::Style);
  });

  auto flagsChange = [this]() {
    FontInfo::Flags flags = FontInfo::Flags::None;
    if (m_antialias.isSelected())
      flags |= FontInfo::Flags::Antialias;
    if (m_ligatures.getItem(0)->isSelected())
      flags |= FontInfo::Flags::Ligatures;
    setInfo(FontInfo(m_info, m_info.size(), m_info.style(), flags),
            From::Flags);
  };
  m_ligatures.ItemChange.connect(flagsChange);
  m_antialias.Click.connect(flagsChange);
}

// Defined here as FontPopup type is not fully defined in the header
// file (and we have a std::unique_ptr<FontPopup> in FontEntry::FontFace).
FontEntry::~FontEntry()
{
}

void FontEntry::setInfo(const FontInfo& info,
                        const From fromField)
{
  m_info = info;

  if (fromField != From::Face)
    m_face.setText(info.title());

  if (fromField != From::Size)
    m_size.setValue(fmt::format("{}", info.size()));

  if (fromField != From::Style) {
    m_style.getItem(0)->setSelected(info.style().weight() >= text::FontStyle::Weight::SemiBold);
    m_style.getItem(1)->setSelected(info.style().slant() != text::FontStyle::Slant::Upright);
  }

  if (fromField != From::Flags) {
    m_ligatures.getItem(0)->setSelected(info.ligatures());
    m_antialias.setSelected(info.antialias());
  }

  FontChange(m_info, fromField);
}

} // namespace app
