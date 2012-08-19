/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef OBSERVABLE_H_INCLUDED
#define OBSERVABLE_H_INCLUDED

#include "observers.h"

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
    m_observers.notifyObservers<A1>(method, a1);
  }

  template<typename A1, typename A2>
  void notifyObservers(void (Observer::*method)(A1, A2), A1 a1, A2 a2) {
    m_observers.notifyObservers<A1, A2>(method, a1, a2);
  }

  template<typename A1, typename A2, typename A3>
  void notifyObservers(void (Observer::*method)(A1, A2, A3), A1 a1, A2 a2, A3 a3) {
    m_observers.notifyObservers<A1, A2, A3>(method, a1, a2, a3);
  }

private:
  Observers<Observer> m_observers;
};

#endif  // OBSERVERS_H_INCLUDED
