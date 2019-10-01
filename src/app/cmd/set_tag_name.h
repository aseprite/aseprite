// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_TAG_NAME_H_INCLUDED
#define APP_CMD_SET_TAG_NAME_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_tag.h"

#include <string>

namespace app {
namespace cmd {
  using namespace doc;

  class SetTagName : public Cmd
                   , public WithTag {
  public:
    SetTagName(Tag* tag, const std::string& name);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override {
      return sizeof(*this);
    }

  private:
    std::string m_oldName;
    std::string m_newName;
  };

} // namespace cmd
} // namespace app

#endif
