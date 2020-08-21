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
#include "app/ui/user_data_popup.h"
#include "base/clamp.h"
#include "doc/sprite.h"
#include "doc/tag.h"

namespace app {

TagWindow::TagWindow(const doc::Sprite* sprite, const doc::Tag* tag)
  : m_sprite(sprite)
  , m_base(Preferences::instance().document(
     static_cast<Doc*>(sprite->document())).timeline.firstFrame())
  , m_userData(tag->userData())
{
  name()->setText(tag->name());
  from()->setTextf("%d", tag->fromFrame()+m_base);
  to()->setTextf("%d", tag->toFrame()+m_base);
  color()->setColor(app::Color::fromRgb(
      doc::rgba_getr(tag->color()),
      doc::rgba_getg(tag->color()),
      doc::rgba_getb(tag->color())));

  fill_anidir_combobox(anidir(), tag->aniDir());
  userData()->Click.connect([this]{ onPopupUserData(); });
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

doc::AniDir TagWindow::aniDirValue()
{
  return (doc::AniDir)anidir()->getSelectedItemIndex();
}

void TagWindow::onPopupUserData()
{
  // Required because we are showing the color in the TagWindow so we
  // have to put the color in the user data before showing the same
  // color again in the user data tooltip.
  //
  // TODO remove this field in the future
  app::Color c = color()->getColor();
  m_userData.setColor(doc::rgba(c.getRed(),
                                c.getGreen(),
                                c.getBlue(),
                                c.getAlpha()));

  if (show_user_data_popup(userData()->bounds(), m_userData)) {
    color_t d = m_userData.color();
    // And synchronize back the color from the user data with the
    // color in the tag properties color picker.
    color()->setColor(
      app::Color::fromRgb(int(rgba_getr(d)),
                          int(rgba_getg(d)),
                          int(rgba_getb(d)),
                          int(rgba_geta(d))));
  }
}

} // namespace app
