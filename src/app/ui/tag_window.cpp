// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
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
#include "app/ui/user_data_view.h"
#include "base/convert_to.h"
#include "doc/sprite.h"
#include "doc/tag.h"
#include "ui/manager.h"
#include "ui/message.h"

#include <algorithm>

namespace app {

const char* kInfiniteSymbol = "\xE2\x88\x9E"; // Infinite symbol (UTF-8)

TagWindow::Repeat::Repeat()
{
}

bool TagWindow::Repeat::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {
    case ui::kFocusEnterMessage: {
      if (text() == kInfiniteSymbol)
        setText("");
      break;
    }
  }
  return ExprEntry::onProcessMessage(msg);
}

void TagWindow::Repeat::onFormatExprFocusLeave(std::string& buf)
{
  ExprEntry::onFormatExprFocusLeave(buf);
  if (buf.empty() || base::convert_to<int>(buf) == 0)
    buf = kInfiniteSymbol;
}

TagWindow::TagWindow(const doc::Sprite* sprite, const doc::Tag* tag)
  : m_sprite(sprite)
  , m_base(
      Preferences::instance().document(static_cast<Doc*>(sprite->document())).timeline.firstFrame())
  , m_userData(tag->userData())
  , m_userDataView(Preferences::instance().tags.userDataVisibility)
{
  m_userDataView.configureAndSet(m_userData, propertiesGrid());

  repeatPlaceholder()->addChild(&m_repeat);

  name()->setText(tag->name());
  from()->setTextf("%d", tag->fromFrame() + m_base);
  to()->setTextf("%d", tag->toFrame() + m_base);

  if (tag->repeat() > 0) {
    limitRepeat()->setSelected(true);
    repeat()->setTextf("%d", tag->repeat());
  }
  else {
    limitRepeat()->setSelected(false);
    repeat()->setText(kInfiniteSymbol);
  }

  fill_anidir_combobox(anidir(), tag->aniDir());

  limitRepeat()->Click.connect([this] { onLimitRepeat(); });
  repeat()->Change.connect([this] { onRepeatChange(); });
  userData()->Click.connect([this] { onToggleUserData(); });
}

bool TagWindow::show()
{
  openWindowInForeground();
  return (closer() == ok());
}

std::string TagWindow::nameValue() const
{
  return name()->text();
}

void TagWindow::rangeValue(doc::frame_t& from, doc::frame_t& to) const
{
  doc::frame_t first = 0;
  doc::frame_t last = m_sprite->lastFrame();

  from = this->from()->textInt() - m_base;
  to = this->to()->textInt() - m_base;
  from = std::clamp(from, first, last);
  to = std::clamp(to, from, last);
}

doc::AniDir TagWindow::aniDirValue() const
{
  return (doc::AniDir)anidir()->getSelectedItemIndex();
}

int TagWindow::repeatValue() const
{
  return repeat()->textInt();
}

void TagWindow::onLimitRepeat()
{
  if (limitRepeat()->isSelected())
    repeat()->setText("1");
  else
    repeat()->setText(kInfiniteSymbol);
}

void TagWindow::onRepeatChange()
{
  if (repeat()->text().empty() || repeat()->textInt() == 0)
    limitRepeat()->setSelected(false);
  else
    limitRepeat()->setSelected(true);
}

void TagWindow::onToggleUserData()
{
  m_userDataView.toggleVisibility();
  remapWindow();
  manager()->invalidate();
}

} // namespace app
