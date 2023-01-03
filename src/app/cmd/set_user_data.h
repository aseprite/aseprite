// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_USER_DATA_H_INCLUDED
#define APP_CMD_SET_USER_DATA_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_document.h"
#include "doc/object_id.h"
#include "doc/user_data.h"

namespace doc {
  class WithUserData;
}

namespace app {
namespace cmd {

  class SetUserData : public Cmd
                    , public WithDocument {
  public:
    SetUserData(doc::WithUserData* obj,
                const doc::UserData& userData,
                app::Doc* doc);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onFireNotifications() override;
    size_t onMemSize() const override {
      return sizeof(*this) +
        m_oldUserData.size() +
        m_newUserData.size();
    }

  private:
    doc::ObjectId m_objId;
    doc::UserData m_oldUserData;
    doc::UserData m_newUserData;
  };

} // namespace cmd
} // namespace app

#endif
