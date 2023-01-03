// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_USER_DATA_PROPERTY_H_INCLUDED
#define APP_CMD_SET_USER_DATA_PROPERTY_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "doc/object_id.h"
#include "doc/user_data.h"

namespace doc {
  class WithUserData;
}

namespace app {
namespace cmd {

  class SetUserDataProperty : public Cmd {
  public:
    SetUserDataProperty(
      doc::WithUserData* obj,
      const std::string& group,
      const std::string& field,
      doc::UserData::Variant&& newValue);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override {
      return sizeof(*this);     // TODO + variant size
    }

  private:
    doc::ObjectId m_objId;
    std::string m_group;
    std::string m_field;
    doc::UserData::Variant m_oldValue;
    doc::UserData::Variant m_newValue;
  };

} // namespace cmd
} // namespace app

#endif
