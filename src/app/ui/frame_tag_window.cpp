// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/frame_tag_window.h"

#include "app/doc.h"
#include "app/pref/preferences.h"
#include "app/ui/layer_frame_comboboxes.h"
#include "doc/frame_tag.h"
#include "doc/sprite.h"

namespace app {

FrameTagWindow::FrameTagWindow(const doc::Sprite* sprite, const doc::FrameTag* frameTag)
  : m_sprite(sprite)
  , m_base(Preferences::instance().document(
     static_cast<Doc*>(sprite->document())).timeline.firstFrame())
{
  name()->setText(frameTag->name());
  from()->setTextf("%d", frameTag->fromFrame()+m_base);
  to()->setTextf("%d", frameTag->toFrame()+m_base);
  color()->setColor(app::Color::fromRgb(
      doc::rgba_getr(frameTag->color()),
      doc::rgba_getg(frameTag->color()),
      doc::rgba_getb(frameTag->color())));

  fill_anidir_combobox(anidir(), frameTag->aniDir());
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

  from = this->from()->textInt()-m_base;
  to   = this->to()->textInt()-m_base;
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
