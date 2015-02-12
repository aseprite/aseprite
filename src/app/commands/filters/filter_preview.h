// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_COMMANDS_FILTERS_FILTER_PREVIEW_H_INCLUDED
#define APP_COMMANDS_FILTERS_FILTER_PREVIEW_H_INCLUDED
#pragma once

#include "ui/timer.h"
#include "ui/widget.h"

namespace app {

  class FilterManagerImpl;

  // Invisible widget to control a effect-preview in the current editor.
  class FilterPreview : public ui::Widget {
  public:
    FilterPreview(FilterManagerImpl* filterMgr);
    ~FilterPreview();

    void stop();
    void restartPreview();
    FilterManagerImpl* getFilterManager() const;

  protected:
    bool onProcessMessage(ui::Message* msg) override;

  private:
    FilterManagerImpl* m_filterMgr;
    ui::Timer m_timer;
  };

} // namespace app

#endif
