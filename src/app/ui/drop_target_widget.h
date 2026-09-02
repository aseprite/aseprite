// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_DROP_TARGET_WIDGET_H_INCLUDED
#define APP_UI_DROP_TARGET_WIDGET_H_INCLUDED
#pragma once

#include "gfx/point.h"
#include "ui/widget.h"

#ifdef _DEBUG
  #include "base/log.h"
#endif

namespace app {

// This class is used in conjunction with DraggableWidget
class DropTargetWidget {
public:
  virtual void onDropWidget(const gfx::Point& mousePos, ui::Widget* widget, bool inside)
  {
#ifdef _DEBUG
    LOG(VERBOSE,
        "UI: [id=%s, type=%d]: onDropWidget(), position: (%d, %d)\n",
        this->widget()->id().c_str(),
        this->widget()->type(),
        this->widget()->bounds().x,
        this->widget()->bounds().y);
#endif
  }

  virtual void onDragWidgetEnter(const gfx::Point& mousePos)
  {
#ifdef _DEBUG
    LOG(VERBOSE,
        "UI: [id=%s, type=%d]: onDragWidgetEnter(), position: (%d, %d)\n",
        widget()->id().c_str(),
        widget()->type(),
        widget()->bounds().x,
        widget()->bounds().y);
#endif
  }

  virtual bool onDragWidgetOver(const gfx::Point& mousePos, ui::Widget* widget)
  {
#ifdef _DEBUG
    LOG(VERBOSE,
        "UI: [id=%s, type=%d]: onDragWidgetOver(), position: (%d, %d)\n",
        this->widget()->id().c_str(),
        this->widget()->type(),
        this->widget()->bounds().x,
        this->widget()->bounds().y);
#endif
    return true;
  }

  virtual void onDragWidgetLeave(const gfx::Point& mousePos)
  {
#ifdef _DEBUG
    LOG(VERBOSE,
        "UI: [id=%s, type=%d]: onDragWidgetLeave(), position: (%d, %d)\n",
        widget()->id().c_str(),
        widget()->type(),
        widget()->bounds().x,
        widget()->bounds().y);
#endif
  }

#ifdef _DEBUG
private:
  ui::Widget* widget() { return dynamic_cast<ui::Widget*>(this); }
#endif
};

} // namespace app

#endif
