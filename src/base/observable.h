// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_OBSERVABLE_H_INCLUDED
#define BASE_OBSERVABLE_H_INCLUDED
#pragma once

#include "base/observers.h"

namespace base {

template<typename Observer>
class Observable {
public:

  void addObserver(Observer* observer) {
    m_observers.addObserver(observer);
  }

  void removeObserver(Observer* observer) {
    m_observers.removeObserver(observer);
  }

  void notifyObservers(void (Observer::*method)()) {
    m_observers.notifyObservers(method);
  }

  template<typename A1>
  void notifyObservers(void (Observer::*method)(A1), A1 a1) {
    m_observers.template notifyObservers<A1>(method, a1);
  }

  template<typename A1, typename A2>
  void notifyObservers(void (Observer::*method)(A1, A2), A1 a1, A2 a2) {
    m_observers.template notifyObservers<A1, A2>(method, a1, a2);
  }

  template<typename A1, typename A2, typename A3>
  void notifyObservers(void (Observer::*method)(A1, A2, A3), A1 a1, A2 a2, A3 a3) {
    m_observers.template notifyObservers<A1, A2, A3>(method, a1, a2, a3);
  }

private:
  Observers<Observer> m_observers;
};

} // namespace base

#endif  // BASE_OBSERVERS_H_INCLUDED
