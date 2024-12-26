// Aseprite
// Copyright (C) 2021  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_TAG_REPEAT_H_INCLUDED
#define APP_CMD_SET_TAG_REPEAT_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_tag.h"

namespace app { namespace cmd {
using namespace doc;

class SetTagRepeat : public Cmd,
                     public WithTag {
public:
  SetTagRepeat(Tag* tag, int repeat);

protected:
  void onExecute() override;
  void onUndo() override;
  size_t onMemSize() const override { return sizeof(*this); }

private:
  int m_oldRepeat;
  int m_newRepeat;
};

}} // namespace app::cmd

#endif
