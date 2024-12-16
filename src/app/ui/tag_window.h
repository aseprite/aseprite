// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_TAG_WINDOW_H_INCLUDED
#define APP_UI_TAG_WINDOW_H_INCLUDED
#pragma once

#include "app/ui/color_button.h"
#include "app/ui/expr_entry.h"
#include "app/ui/user_data_view.h"
#include "doc/anidir.h"
#include "doc/frame.h"
#include "doc/user_data.h"

#include "tag_properties.xml.h"

namespace doc {
class Sprite;
class Tag;
} // namespace doc

namespace app {

class TagWindow : protected app::gen::TagProperties {
public:
  TagWindow(const doc::Sprite* sprite, const doc::Tag* tag);

  bool show();

  std::string nameValue() const;
  void rangeValue(doc::frame_t& from, doc::frame_t& to) const;
  doc::AniDir aniDirValue() const;
  int repeatValue() const;
  const doc::UserData& userDataValue() const { return m_userDataView.userData(); }

private:
  class Repeat : public ExprEntry {
  public:
    Repeat();

  private:
    bool onProcessMessage(ui::Message* msg) override;
    void onFormatExprFocusLeave(std::string& buf) override;
  };

  const Repeat* repeat() const { return &m_repeat; }
  Repeat* repeat() { return &m_repeat; }

  void onLimitRepeat();
  void onRepeatChange();
  void onToggleUserData();

  const doc::Sprite* m_sprite;
  int m_base;
  doc::UserData m_userData;
  UserDataView m_userDataView;
  Repeat m_repeat;
};

} // namespace app

#endif
