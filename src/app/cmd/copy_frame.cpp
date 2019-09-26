// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/copy_frame.h"

#include "app/cmd/add_frame.h"
#include "app/cmd/copy_cel.h"
#include "app/cmd/set_frame_duration.h"
#include "doc/sprite.h"
#include "doc/layer.h"

namespace app {
namespace cmd {

using namespace doc;

CopyFrame::CopyFrame(Sprite* sprite, frame_t fromFrame, frame_t newFrame)
  : WithSprite(sprite)
  , m_fromFrame(fromFrame)
  , m_newFrame(newFrame)
{
}

void CopyFrame::onExecute()
{
  Sprite* sprite = this->sprite();
  frame_t fromFrame = m_fromFrame;
  int msecs = sprite->frameDuration(fromFrame);

  executeAndAdd(new cmd::AddFrame(sprite, m_newFrame));
  executeAndAdd(new cmd::SetFrameDuration(sprite, m_newFrame, msecs));

  // Do not copy cels (cmd::CopyCel must be called from outside)
}

} // namespace cmd
} // namespace app
