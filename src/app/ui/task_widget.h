// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_TASK_WIDGET_H_INCLUDED
#define APP_UI_TASK_WIDGET_H_INCLUDED
#pragma once

#include "app/task.h"
#include "obs/signal.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/slider.h"
#include "ui/timer.h"

namespace app {

class TaskWidget : public ui::Box {
public:
  enum Type {
    kCanCancel = 1,
    kWithProgress = 2,
    kCannotCancel = kWithProgress,
    kCancelAndProgress = 3,
  };

  TaskWidget(const Type type,
             base::task::func_t&& func);
  TaskWidget(base::task::func_t&& func)
    : TaskWidget(kCancelAndProgress, std::move(func)) { }

protected:
  virtual void onComplete();

private:
  ui::Timer m_monitorTimer;
  ui::Button m_cancelButton;
  ui::Slider m_progressBar;
  Task m_task;
};

} // namespace app

#endif
