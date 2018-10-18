// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/assign_color_profile.h"

#include "app/doc.h"
#include "app/doc_event.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

AssignColorProfile::AssignColorProfile(doc::Sprite* sprite, const gfx::ColorSpacePtr& cs)
  : WithSprite(sprite)
  , m_oldCS(sprite->colorSpace())
  , m_newCS(cs)
{
}

void AssignColorProfile::onExecute()
{
  doc::Sprite* spr = sprite();
  spr->setColorSpace(m_newCS);
  spr->incrementVersion();
}

void AssignColorProfile::onUndo()
{
  doc::Sprite* spr = sprite();
  spr->setColorSpace(m_oldCS);
  spr->incrementVersion();
}

void AssignColorProfile::onFireNotifications()
{
  doc::Sprite* sprite = this->sprite();
  Doc* doc = static_cast<Doc*>(sprite->document());
  doc->notifyColorSpaceChanged();
}

} // namespace cmd
} // namespace app
