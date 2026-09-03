// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_CLEAR_IMAGE_H_INCLUDED
#define APP_CMD_CLEAR_IMAGE_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_image.h"
#include "doc/color.h"
#include "doc/image_ref.h"

namespace app { namespace cmd {
using namespace doc;

class ClearImage : public Cmd,
                   public WithImage {
public:
  CMDTYPE2('c', 'l', 'I', 'm', ClearImage);

  ClearImage(Image* image, color_t color);

protected:
  void onExecute(Context* ctx) override;
  void onUndo(Context* ctx) override;
  size_t onMemSize() const override { return sizeof(*this) + (m_copy ? m_copy->getMemSize() : 0); }
  void onSerialize(CmdSerial& s) override;

private:
  ImageRef m_copy;
  color_t m_color;
};

}} // namespace app::cmd

#endif
