// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/frame_tag_window.h"

#include "doc/frame_tag.h"
#include "doc/sprite.h"

namespace app {

FrameTagWindow::FrameTagWindow(const doc::Sprite* sprite, const doc::FrameTag* frameTag)
  : m_sprite(sprite)
{
  name()->setText(frameTag->name());
  from()->setTextf("%d", frameTag->fromFrame()+1);
  to()->setTextf("%d", frameTag->toFrame()+1);
  color()->setColor(app::Color::fromRgb(
      doc::rgba_getr(frameTag->color()),
      doc::rgba_getg(frameTag->color()),
      doc::rgba_getb(frameTag->color())));

  static_assert(
    int(doc::AniDir::FORWARD) == 0 &&
    int(doc::AniDir::REVERSE) == 1 &&
    int(doc::AniDir::PING_PONG) == 2, "doc::AniDir has changed");
  anidir()->addItem("Forward");
  anidir()->addItem("Reverse");
  anidir()->addItem("Ping-pong");
  anidir()->setSelectedItemIndex(int(frameTag->aniDir()));
}

bool FrameTagWindow::show()
{
  openWindowInForeground();
  return (closer() == ok());
}

std::string FrameTagWindow::nameValue()
{
  return name()->text();
}

void FrameTagWindow::rangeValue(doc::frame_t& from, doc::frame_t& to)
{
  doc::frame_t first = 0;
  doc::frame_t last = m_sprite->lastFrame();

  from = this->from()->textInt()-1;
  to   = this->to()->textInt()-1;
  from = MID(first, from, last);
  to   = MID(from, to, last);
}

doc::color_t FrameTagWindow::colorValue()
{
  app::Color color = this->color()->getColor();
  return doc::rgba(color.getRed(), color.getGreen(), color.getBlue(), 255);
}

doc::AniDir FrameTagWindow::aniDirValue()
{
  return (doc::AniDir)anidir()->getSelectedItemIndex();
}

} // namespace app
