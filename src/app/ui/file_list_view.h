// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_FILE_LIST_VIEW_H_INCLUDED
#define APP_UI_FILE_LIST_VIEW_H_INCLUDED
#pragma once

#include "ui/view.h"

namespace app {

  class FileListView : public ui::View {
  public:
    FileListView() { }

  private:
    void onScrollRegion(ui::ScrollRegionEvent& ev);
  };

} // namespace app

#endif
