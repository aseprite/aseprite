// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cli/cli_open_file.h"

#include "app/document.h"
#include "app/file/file.h"
#include "doc/frame_tags.h"
#include "doc/sprite.h"

namespace app {

CliOpenFile::CliOpenFile()
{
  document = nullptr;
  fromFrame = -1;
  toFrame = -1;
  splitLayers = false;
  allLayers = false;
  listLayers = false;
  listTags = false;
  ignoreEmpty = false;
  trim = false;
  crop = gfx::Rect();
}

FileOpROI CliOpenFile::roi() const
{
  ASSERT(document);

  if (hasFrameTag()) {
    return FileOpROI(
      document,
      document->sprite()->frameTags().getByName(frameTag));
  }
  else if (hasFrameRange()) {
    return FileOpROI(document, fromFrame, toFrame);
  }
  else {
    return FileOpROI(document);
  }
}

} // namespace app
