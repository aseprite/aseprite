// Aseprite
// Copyright (C) 2025-2026  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_CEL_IMAGE_H_INCLUDED
#define APP_CMD_SET_CEL_IMAGE_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_cel.h"
#include "app/cmd/with_suspended.h"
#include "doc/image_ref.h"

#include <sstream>

namespace app { namespace cmd {

class SetCelImage : public Cmd,
                    public WithCel {
public:
  SetCelImage(doc::Cel* cel, const doc::ImageRef& newImage);

protected:
  void onExecute() override;
  void onUndo() override;
  void onRedo() override;
  size_t onMemSize() const override { return sizeof(*this); }

private:
  doc::ImageRef m_newImage;
  WithSuspended<doc::ImageRef> m_suspendedImage;
  gfx::Rect m_bounds;
};

}} // namespace app::cmd

#endif
