// Aseprite
// Copyright (C) 2022-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_FILTERS_FILTER_PREVIEW_H_INCLUDED
#define APP_COMMANDS_FILTERS_FILTER_PREVIEW_H_INCLUDED
#pragma once

#include "ui/timer.h"
#include "ui/widget.h"

#include <mutex>
#include <thread>

namespace app {

  class FilterManagerImpl;

  // Invisible widget to control a effect-preview in the current editor.
  class FilterPreview : public ui::Widget {
  public:
    FilterPreview(FilterManagerImpl* filterMgr);
    ~FilterPreview();

    void setEnablePreview(bool state);

    void stop();
    void restartPreview();

  protected:
    bool onProcessMessage(ui::Message* msg) override;

  private:
    void onFilterThread();

    FilterManagerImpl* m_filterMgr;
    ui::Timer m_timer;
    std::mutex m_filterMgrMutex;
    std::unique_ptr<std::thread> m_filterThread;
    bool m_filterIsDone;
  };

} // namespace app

#endif
