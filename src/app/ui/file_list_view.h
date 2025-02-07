// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_FILE_LIST_VIEW_H_INCLUDED
#define APP_UI_FILE_LIST_VIEW_H_INCLUDED
#pragma once

#include "ui/view.h"

namespace app {

class FileListView : public ui::View {
public:
  FileListView() {}

private:
  void onScrollRegion(ui::ScrollRegionEvent& ev);
};

} // namespace app

#endif
