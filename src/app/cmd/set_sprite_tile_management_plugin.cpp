// Aseprite
// Copyright (c) 2023-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/set_sprite_tile_management_plugin.h"

#include "app/doc.h"
#include "app/doc_event.h"
#include "doc/sprite.h"

namespace app { namespace cmd {

SetSpriteTileManagementPlugin::SetSpriteTileManagementPlugin(Sprite* sprite,
                                                             const std::string& value)
  : WithSprite(sprite)
  , m_oldValue(sprite->tileManagementPlugin())
  , m_newValue(value)
{
}

void SetSpriteTileManagementPlugin::onExecute(Context* ctx)
{
  Sprite* spr = sprite();
  spr->setTileManagementPlugin(m_newValue);
  spr->incrementVersion();
}

void SetSpriteTileManagementPlugin::onUndo(Context* ctx)
{
  Sprite* spr = sprite();
  spr->setTileManagementPlugin(m_oldValue);
  spr->incrementVersion();
}

void SetSpriteTileManagementPlugin::onFireNotifications(Context* ctx)
{
  Sprite* spr = sprite();
  Doc* doc = static_cast<Doc*>(spr->document());
  DocEvent ev(doc);
  ev.sprite(spr);
  doc->notify_observers<DocEvent&>(&DocObserver::onTileManagementPluginChange, ev);
}

}} // namespace app::cmd
