// Aseprite
// Copyright (C) 2021  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_TIMELINE_TIMELINE_OBSERVER_H_INCLUDED
#define APP_UI_TIMELINE_TIMELINE_OBSERVER_H_INCLUDED
#pragma once

namespace app {
class Timeline;

class TimelineObserver {
public:
  virtual ~TimelineObserver() {}

  // Called when the current timeline range is going to change.
  virtual void onBeforeRangeChanged(Timeline* timeline) {}
};

} // namespace app

#endif
