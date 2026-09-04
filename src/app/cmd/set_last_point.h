// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_LAST_POINT_H_INCLUDED
#define APP_CMD_SET_LAST_POINT_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_document.h"
#include "gfx/point.h"

namespace app { namespace cmd {
using namespace doc;

class SetLastPoint : public Cmd,
                     public WithDocument {
public:
  CMDTYPE('L', 'a', 'P', 't');

  SetLastPoint(Doc* doc, const gfx::Point& pos);

protected:
  void onExecute(Context* ctx) override;
  void onUndo(Context* ctx) override;
  size_t onMemSize() const override { return sizeof(*this); }

  bool onIsSerializable() const override
  {
    // Don't serialize (this Cmd is for UI purposes only)
    return false;
  }

private:
  void setLastPoint(const gfx::Point& pos);

  gfx::Point m_oldPoint;
  gfx::Point m_newPoint;
};

}} // namespace app::cmd

#endif
