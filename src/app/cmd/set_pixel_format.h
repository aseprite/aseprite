// Aseprite
// Copyright (C) 2019-2024  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_PIXEL_FORMAT_H_INCLUDED
#define APP_CMD_SET_PIXEL_FORMAT_H_INCLUDED
#pragma once

#include "app/cmd/with_sprite.h"
#include "app/cmd_sequence.h"
#include "doc/color.h"
#include "doc/fit_criteria.h"
#include "doc/frame.h"
#include "doc/image_ref.h"
#include "doc/pixel_format.h"
#include "doc/rgbmap_algorithm.h"

namespace doc {
class Sprite;
}

namespace render {
class Dithering;
class TaskDelegate;
} // namespace render

namespace app { namespace cmd {

class SetPixelFormat : public Cmd,
                       public WithSprite {
public:
  SetPixelFormat(doc::Sprite* sprite,
                 const doc::PixelFormat newFormat,
                 const render::Dithering& dithering,
                 const doc::RgbMapAlgorithm mapAlgorithm,
                 doc::rgba_to_graya_func toGray,
                 render::TaskDelegate* delegate,
                 const doc::FitCriteria fitCriteria);

protected:
  void onExecute() override;
  void onUndo() override;
  void onRedo() override;
  size_t onMemSize() const override { return sizeof(*this) + m_pre.memSize() + m_post.memSize(); }

private:
  void setFormat(doc::PixelFormat format);
  void convertImage(doc::Sprite* sprite,
                    const render::Dithering& dithering,
                    const doc::ImageRef& oldImage,
                    const doc::frame_t frame,
                    const bool isBackground,
                    const doc::RgbMapAlgorithm mapAlgorithm,
                    doc::rgba_to_graya_func toGray,
                    render::TaskDelegate* delegate,
                    const doc::FitCriteria fitCriteria = doc::FitCriteria::DEFAULT);

  doc::PixelFormat m_oldFormat;
  doc::PixelFormat m_newFormat;
  CmdSequence m_pre;
  CmdSequence m_post;
};

}} // namespace app::cmd

#endif
