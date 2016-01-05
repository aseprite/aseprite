// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/file_list_view.h"

#include "app/ui/file_list.h"
#include "ui/scroll_region_event.h"

namespace app {

void FileListView::onScrollRegion(ui::ScrollRegionEvent& ev)
{
  View::onScrollRegion(ev);

  if (auto fileList = dynamic_cast<FileList*>(attachedWidget())) {
    gfx::Rect tbounds = fileList->thumbnailBounds();
    if (!tbounds.isEmpty()) {
      tbounds
        .enlarge(1)
        .offset(fileList->bounds().origin());

      ev.region().createSubtraction(ev.region(), gfx::Region(tbounds));
    }
  }
}

} // namespace app
