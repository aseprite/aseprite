// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/tag_window.h"

#include "app/doc.h"
#include "app/pref/preferences.h"
#include "app/ui/layer_frame_comboboxes.h"
#include "base/clamp.h"
#include "doc/sprite.h"
#include "doc/tag.h"

namespace app {

TagWindow::TagWindow(const doc::Sprite* sprite, const doc::Tag* tag)
  : m_sprite(sprite)
  , m_base(Preferences::instance().document(
     static_cast<Doc*>(sprite->document())).timeline.firstFrame())
{
  name()->setText(tag->name());
  from()->setTextf("%d", tag->fromFrame()+m_base);
  to()->setTextf("%d", tag->toFrame()+m_base);
  color()->setColor(app::Color::fromRgb(
      doc::rgba_getr(tag->color()),
      doc::rgba_getg(tag->color()),
      doc::rgba_getb(tag->color())));

  fill_anidir_combobox(anidir(), tag->aniDir());
}

bool TagWindow::show()
{
  openWindowInForeground();
  return (closer() == ok());
}

std::string TagWindow::nameValue()
{
  return name()->text();
}

void TagWindow::rangeValue(doc::frame_t& from, doc::frame_t& to)
{
  doc::frame_t first = 0;
  doc::frame_t last = m_sprite->lastFrame();

  from = this->from()->textInt()-m_base;
  to   = this->to()->textInt()-m_base;
  from = base::clamp(from, first, last);
  to   = base::clamp(to, from, last);
}

doc::color_t TagWindow::colorValue()
{
  app::Color color = this->color()->getColor();
  return doc::rgba(color.getRed(), color.getGreen(), color.getBlue(), 255);
}

doc::AniDir TagWindow::aniDirValue()
{
  return (doc::AniDir)anidir()->getSelectedItemIndex();
}

} // namespace app
