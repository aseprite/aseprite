// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <cstdlib>
#include <string>

#include "app/ui/hex_color_entry.h"
#include "base/hex.h"
#include "gfx/border.h"
#include "ui/message.h"
#include "ui/scale.h"

namespace app {

using namespace ui;

HexColorEntry::CustomEntry::CustomEntry() : Entry(16, "")
{
}

bool HexColorEntry::CustomEntry::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {
    case kMouseDownMessage:
      setFocusStop(true);
      requestFocus();
      break;
    case kFocusLeaveMessage: setFocusStop(false); break;
  }
  return Entry::onProcessMessage(msg);
}

HexColorEntry::HexColorEntry() : Box(HORIZONTAL), m_label("#")
{
  addChild(&m_label);
  addChild(&m_entry);

  m_entry.Change.connect(&HexColorEntry::onEntryChange, this);
  m_entry.setFocusStop(false);

  initTheme();

  setBorder(gfx::Border(2 * ui::guiscale(), 0, 0, 0));
  setChildSpacing(0);
}

void HexColorEntry::setColor(const app::Color& color)
{
  m_entry.setTextf("%02x%02x%02x", color.getRed(), color.getGreen(), color.getBlue());
}

void HexColorEntry::onEntryChange()
{
  std::string text = m_entry.text();
  int r, g, b;

  // Remove non hex digits
  while (text.size() > 0 && !base::is_hex_digit(text[0]))
    text.erase(0, 1);

  // Fill with zeros at the end of the text
  while (text.size() < 6)
    text.push_back('0');

  // Convert text (Base 16) to integer
  int hex = std::strtol(text.c_str(), NULL, 16);

  r = (hex & 0xff0000) >> 16;
  g = (hex & 0xff00) >> 8;
  b = (hex & 0xff);

  ColorChange(app::Color::fromRgb(r, g, b));
}

} // namespace app
