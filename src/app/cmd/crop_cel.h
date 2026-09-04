// Aseprite
// Copyright (C) 2019-present  Igara Studio S.A.
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_CROP_CEL_H_INCLUDED
#define APP_CMD_CROP_CEL_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_cel.h"
#include "gfx/point.h"
#include "gfx/rect.h"

namespace app { namespace cmd {

class CropCel : public Cmd,
                public WithCel {
public:
  CMDTYPE2('c', 'r', 'o', 'p', CropCel);

  CropCel(doc::Cel* cel, const gfx::Rect& newBounds);

protected:
  void onExecute(Context* ctx) override;
  void onUndo(Context* ctx) override;
  void onFireNotifications(Context* ctx) override;
  size_t onMemSize() const override { return sizeof(*this); }
  void onSerialize(CmdSerial& s) override;

private:
  void cropImage(const gfx::Point& origin, const gfx::Rect& bounds);

  gfx::Point m_oldOrigin;
  gfx::Point m_newOrigin;
  gfx::Rect m_oldBounds;
  gfx::Rect m_newBounds;
};

}} // namespace app::cmd

#endif
