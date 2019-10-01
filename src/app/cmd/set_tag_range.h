// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_TAG_RANGE_H_INCLUDED
#define APP_CMD_SET_TAG_RANGE_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_tag.h"
#include "doc/frame.h"

namespace app {
namespace cmd {
  using namespace doc;

  class SetTagRange : public Cmd
                    , public WithTag {
  public:
    SetTagRange(Tag* tag, frame_t from, frame_t to);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override {
      return sizeof(*this);
    }

  private:
    frame_t m_oldFrom, m_oldTo;
    frame_t m_newFrom, m_newTo;
  };

} // namespace cmd
} // namespace app

#endif
