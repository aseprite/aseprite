// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_REMOVE_USER_DATA_PROPERTY_H_INCLUDED
#define APP_CMD_REMOVE_USER_DATA_PROPERTY_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "doc/object_id.h"
#include "doc/user_data.h"

namespace doc {
  class WithUserData;
}

namespace app {
namespace cmd {

  class RemoveUserDataProperty : public Cmd {
  public:
    RemoveUserDataProperty(
      doc::WithUserData* obj,
      const std::string& group,
      const std::string& field);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override {
      return sizeof(*this);     // TODO + variants size
    }

  private:
    doc::ObjectId m_objId;
    std::string m_group;
    std::string m_field;
    doc::UserData::Variant m_oldVariant;
  };

} // namespace cmd
} // namespace app

#endif
