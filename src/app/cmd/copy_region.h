// Aseprite
// Copyright (C) 2019-present  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_COPY_REGION_H_INCLUDED
#define APP_CMD_COPY_REGION_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_image.h"
#include "base/buffer.h"
#include "doc/tile.h"
#include "gfx/point.h"
#include "gfx/region.h"

namespace doc {
class Tileset;
}

namespace app { namespace cmd {
using namespace doc;

class CopyRegion : public Cmd,
                   public WithImage {
public:
  CMDTYPE2('c', 'p', 'R', 'g', CopyRegion);

  // If alreadyCopied is false, it means that onExecute() will copy
  // pixels from src to dst. If it's true, it means that "onExecute"
  // should do nothing, because modified pixels are alreadt on "dst"
  // (so we use "src" as the original image).
  CopyRegion(Image* dst,
             const Image* src,
             const gfx::Region& region,
             const gfx::Point& dstPos,
             bool alreadyCopied = false);

protected:
  void onExecute(Context* ctx) override;
  void onUndo(Context* ctx) override;
  void onRedo(Context* ctx) override;
  void onFireNotifications(Context* ctx) override;
  size_t onMemSize() const override { return sizeof(*this) + m_buffer.size(); }
  void onSerialize(CmdSerial& s) override;

private:
  void swap();
  virtual void rehash() {}

  bool m_alreadyCopied;
  gfx::Region m_region;
  gfx::Point m_dstPos;
  base::buffer m_buffer;
};

class CopyTileRegion : public CopyRegion {
public:
  CMDTYPE('c', 'p', 'T', 'r');

  CopyTileRegion(Image* dst,
                 const Image* src,
                 const gfx::Region& region,
                 const gfx::Point& dstPos,
                 bool alreadyCopied,
                 const doc::tile_index tileIndex,
                 const doc::Tileset* tileset);

private:
  void rehash() override;

  doc::tile_index m_tileIndex;
  doc::ObjectId m_tilesetId;
};

}} // namespace app::cmd

#endif
