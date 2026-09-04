// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_SLICE_NAME_H_INCLUDED
#define APP_CMD_SET_SLICE_NAME_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_slice.h"

#include <string>

namespace app { namespace cmd {
using namespace doc;

class SetSliceName : public Cmd,
                     public WithSlice {
public:
  SetSliceName(Slice* slice, const std::string& name);

protected:
  void onExecute(Context* ctx) override;
  void onUndo(Context* ctx) override;
  void onFireNotifications(Context* ctx) override;
  size_t onMemSize() const override { return sizeof(*this) + m_oldName.size() + m_newName.size(); }

  std::string m_oldName;
  std::string m_newName;
};

}} // namespace app::cmd

#endif
