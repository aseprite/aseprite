// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_USER_DATA_VIEW_H_INCLUDED
#define APP_UI_USER_DATA_VIEW_H_INCLUDED
#pragma once

#include "app/pref/preferences.h"
#include "app/ui/color_button.h"
#include "doc/user_data.h"
#include "ui/base.h"
#include "ui/entry.h"
#include "ui/grid.h"
#include "ui/label.h"

#include "user_data.xml.h"

namespace app {

  class UserDataView {
  public:
    UserDataView(gen::UserData* userDataWidgetsContainer, Option<bool>* visibility);

    void configureAndSet(doc::UserData& userData, ui::Grid* parent);
    void toggleVisibility();
    void setVisible(bool state, bool saveAsDefault = true);
    void freeUserDataWidgets(ui::Grid* parent);

    const doc::UserData& userData() const { return m_userData; }
    ColorButton* color() { return m_userDataWidgetsContainer->color(); }
    ui::Entry* entry() { return m_userDataWidgetsContainer->entry();}
    ui::Label* colorLabel() { return m_userDataWidgetsContainer->colorLabel(); }
    ui::Label* entryLabel() { return m_userDataWidgetsContainer->entryLabel(); }

  private:
    bool isVisible() const { return (*m_visibility)(); }
    void onUserDataChange();
    void onColorChange();
    bool isConfigured() const { return m_isConfigured; }

    doc::UserData m_userData;
    Option<bool>* m_visibility;
    gen::UserData* m_userDataWidgetsContainer;
    bool m_isConfigured;
  };

} // namespace app

#endif
