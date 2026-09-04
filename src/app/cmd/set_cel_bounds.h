// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_CEL_BOUNDS_H_INCLUDED
#define APP_CMD_SET_CEL_BOUNDS_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_cel.h"
#include "gfx/rect.h"

namespace app { namespace cmd {
using namespace doc;

class SetCelBoundsF : public Cmd,
                      public WithCel {
public:
  SetCelBoundsF(Cel* cel, const gfx::RectF& bounds);

protected:
  void onExecute(Context* ctx) override;
  void onUndo(Context* ctx) override;
  size_t onMemSize() const override { return sizeof(*this); }

private:
  void setBounds(const gfx::RectF& newBounds);

  gfx::RectF m_oldBounds;
  gfx::RectF m_newBounds;
};

}} // namespace app::cmd

#endif
