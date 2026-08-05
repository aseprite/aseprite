// Aseprite
// Copyright (c) 2026-present Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/crash/document_info.h"

#include "app/doc.h"
#include "base/fs.h"
#include "doc/sprite.h"

namespace app::crash {

using namespace doc;

DocumentInfo::DocumentInfo(const Doc* doc)
{
  const doc::Sprite* sprite = doc->sprite();
  docId = doc->id();
  mode = sprite->colorMode();
  width = sprite->width();
  height = sprite->height();
  frames = sprite->totalFrames();
  filename = doc->filename();
}

} // namespace app::crash
