// Aseprite Base Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_TIME_H_INCLUDED
#define BASE_TIME_H_INCLUDED
#pragma once

#include "base/ints.h"

#include <string>

namespace base {

  // ~1000 per second
  typedef uint64_t tick_t;

  class Time {
  public:
    int year, month, day;
    int hour, minute, second;

    Time(int year = 0, int month = 0, int day = 0,
         int hour = 0, int minute = 0, int second = 0)
      : year(year), month(month), day(day)
      , hour(hour), minute(minute), second(second) {
    }

    bool valid() const {
      return (year != 0 && month != 0 && day != 0);
    }

    void dateOnly() {
      hour = minute = second = 0;
    }

    bool operator==(const Time& other) {
      return
        year == other.year &&
        month == other.month &&
        day == other.day &&
        hour == other.hour &&
        minute == other.minute &&
        second == other.second;
    }

    bool operator!=(const Time& other) {
      return !operator==(other);
    }
  };

  Time current_time();
  tick_t current_tick();

}

#endif
