// Aseprite UI Library
// Copyright (C) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ui/textcmd.h"

#include "ui/clipboard_delegate.h"
#include "ui/menu.h"
#include "ui/message.h"
#include "ui/system.h"
#include "ui/translation_delegate.h"

namespace ui {

TextCmdProcessor::Cmd TextCmdProcessor::cmdFromKeyMessage(const KeyMessage* msg)
{
  switch (msg->scancode()) {
    case kKeyLeft:
      if (msg->ctrlPressed() || msg->altPressed())
        return Cmd::PrevWord;
      if (msg->cmdPressed())
        return Cmd::BegOfLine;
      return Cmd::PrevChar;

    case kKeyRight:
      if (msg->ctrlPressed() || msg->altPressed())
        return Cmd::NextWord;
      if (msg->cmdPressed())
        return Cmd::EndOfLine;
      return Cmd::NextChar;

    case kKeyUp:
      if (msg->cmdPressed())
        return Cmd::BegOfFile;
      return Cmd::PrevLine;

    case kKeyDown:
      if (msg->cmdPressed())
        return Cmd::EndOfFile;
      return Cmd::NextLine;

    case kKeyHome:
      if (msg->ctrlPressed())
        return Cmd::BegOfFile;
      return Cmd::BegOfLine;

    case kKeyEnd:
      if (msg->ctrlPressed())
        return Cmd::EndOfFile;
      return Cmd::EndOfLine;

    case kKeyDel:
      if (msg->shiftPressed()) {
        if (msg->ctrlPressed())
          return Cmd::DeleteToEndOfLine;
        if (onHasValidSelection())
          return Cmd::Cut;
      }
      if (msg->ctrlPressed())
        return Cmd::DeleteNextWord;
      return Cmd::DeleteNextChar;

    case kKeyInsert:
      if (msg->shiftPressed())
        return Cmd::Paste;
      if (msg->ctrlPressed())
        return Cmd::Copy;
      break;

    case kKeyBackspace:
      if (msg->ctrlPressed() || msg->altPressed())
        return Cmd::DeletePrevWord;
      return Cmd::DeletePrevChar;

    default:
      // Map common macOS/Windows shortcuts for Cut/Copy/Paste/Select all
#if LAF_MACOS
      if (msg->onlyCmdPressed())
#else
      if (msg->onlyCtrlPressed())
#endif
      {
        switch (msg->scancode()) {
          case kKeyX: return Cmd::Cut;
          case kKeyC: return Cmd::Copy;
          case kKeyV: return Cmd::Paste;
          case kKeyA: return Cmd::SelectAll;
        }
      }
      break;
  }
  return Cmd::NoOp;
}

void TextCmdProcessor::executeCmd(const Cmd cmd,
                                  const base::codepoint_t unicodeChar,
                                  const bool expandSelection)
{
  onExecuteCmd(cmd, unicodeChar, expandSelection);
}

void TextCmdProcessor::showEditPopupMenu(Display* display, const gfx::Point& pt)
{
  auto* clipboard = UISystem::instance()->clipboardDelegate();
  if (!clipboard)
    return;

  auto* translate = UISystem::instance()->translationDelegate();
  ASSERT(translate); // We provide UISystem as default translation delegate
  if (!translate)
    return;

  Menu menu;
  MenuItem cut(translate->cut());
  MenuItem copy(translate->copy());
  MenuItem paste(translate->paste());
  MenuItem selectAll(translate->selectAll());

  menu.addChild(&cut);
  menu.addChild(&copy);
  menu.addChild(&paste);
  menu.addChild(new MenuSeparator);
  menu.addChild(&selectAll);

  for (auto* item : menu.children())
    item->processMnemonicFromText();

  copy.setEnabled(onHasValidSelection());
  cut.setEnabled(onHasValidSelection() && onCanModify());
  paste.setEnabled(clipboard->hasClipboardText() && onCanModify());

  cut.Click.connect([this] { executeCmd(Cmd::Cut); });
  copy.Click.connect([this] { executeCmd(Cmd::Copy); });
  paste.Click.connect([this] { executeCmd(Cmd::Paste); });
  selectAll.Click.connect([this] { executeCmd(Cmd::SelectAll); });

  menu.showPopup(pt, display);
}

} // namespace ui
