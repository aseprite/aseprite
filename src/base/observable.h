// Aseprite Base Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_OBSERVABLE_H_INCLUDED
#define BASE_OBSERVABLE_H_INCLUDED
#pragma once

#include "obs/observable.h"

namespace base {

template<typename Observer>
class Observable : private obs::observable<Observer> {
  typedef obs::observable<Observer> Base;
public:

  void addObserver(Observer* observer) {
    Base::add_observer(observer);
  }

  void removeObserver(Observer* observer) {
    Base::remove_observer(observer);
  }

  void notifyObservers(void (Observer::*method)()) {
    Base::notify_observers(method);
  }

  template<typename ...Args>
  void notifyObservers(void (Observer::*method)(Args...), Args ...args) {
    Base::template notify_observers<Args...>(method, std::forward<Args>(args)...);
  }
};

} // namespace base

#endif  // BASE_OBSERVERS_H_INCLUDED
