// Aseprite Base Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_SIGNAL_H_INCLUDED
#define BASE_SIGNAL_H_INCLUDED
#pragma once

#include "base/connection.h"
#include "base/remove_from_container.h"
#include "base/slot.h"

#include <vector>

namespace base {

class Signal {
public:
  virtual ~Signal() { }
  virtual void disconnectSlot(Slot* slot) = 0;
};

// Signal0_base<R> - Base class to delegate responsibility to
// functions of zero arguments.
template<typename R>
class Signal0_base : public Signal {
public:
  typedef R ReturnType;
  typedef Slot0<R> SlotType;
  typedef std::vector<SlotType*> SlotList;

  Signal0_base() { }
  ~Signal0_base() {
    for (SlotType* slot : m_slots)
      delete slot;
  }

  Signal0_base(const Signal0_base&) { }
  Signal0_base operator=(const Signal0_base&) {
    return *this;
  }

  Connection addSlot(SlotType* slot) {
    m_slots.push_back(slot);
    return Connection(this, slot);
  }

  template<typename F>
  Connection connect(const F& f) {
    return addSlot(new Slot0_fun<R, F>(f));
  }

  template<class T>
  Connection connect(R (T::*m)(), T* t) {
    return addSlot(new Slot0_mem<R, T>(m, t));
  }

  virtual void disconnectSlot(Slot* slot) override {
    base::remove_from_container(m_slots, static_cast<SlotType*>(slot));
  }

protected:
  SlotList m_slots;
};

// Signal0<R>
template<typename R>
class Signal0 : public Signal0_base<R> {
public:
  // GCC needs redefinitions
  typedef Signal0_base<R> Base;
  typedef Slot0<R> SlotType;
  typedef typename Base::SlotList SlotList;

  Signal0() {
  }

  R operator()(R default_result = R()) {
    R result(default_result);
    SlotList copy = Base::m_slots;
    for (typename SlotList::iterator it = copy.begin(), end = copy.end(); it != end; ++it) {
      SlotType* slot = *it;
      result = (*slot)();
    }
    return result;
  }

  template<typename Merger>
  R operator()(R default_result, const Merger& m) {
    R result(default_result);
    Merger merger(m);
    SlotList copy = Base::m_slots;
    for (typename SlotList::iterator it = copy.begin(), end = copy.end(); it != end; ++it) {
      SlotType* slot = *it;
      result = merger(result, (*slot)());
    }
    return result;
  }

};

// Signal0<void>
template<>
class Signal0<void> : public Signal0_base<void> {
public:
  // GCC needs redefinitions
  typedef Signal0_base<void> Base;
  typedef Slot0<void> SlotType;
  typedef Base::SlotList SlotList;

  Signal0() { }

  void operator()() {
    SlotList copy = Base::m_slots;
    for (SlotList::iterator it = copy.begin(), end = copy.end(); it != end; ++it) {
      SlotType* slot = *it;
      (*slot)();
    }
  }

};

// Signal1_base<R, A1> - Base class to delegate responsibility to
// functions of one argument.
template<typename R, typename A1>
class Signal1_base : public Signal {
public:
  typedef R ReturnType;
  typedef Slot1<R, A1> SlotType;
  typedef std::vector<SlotType*> SlotList;

  Signal1_base() { }
  ~Signal1_base() {
    for (SlotType* slot : m_slots)
      delete slot;
  }

  Signal1_base(const Signal1_base&) { }
  Signal1_base operator=(const Signal1_base&) {
    return *this;
  }

  Connection addSlot(SlotType* slot) {
    m_slots.push_back(slot);
    return Connection(this, slot);
  }

  template<typename F>
  Connection connect(const F& f) {
    return addSlot(new Slot1_fun<R, F, A1>(f));
  }

  template<class T>
  Connection connect(R (T::*m)(A1), T* t) {
    return addSlot(new Slot1_mem<R, T, A1>(m, t));
  }

  virtual void disconnectSlot(Slot* slot) override {
    base::remove_from_container(m_slots, static_cast<SlotType*>(slot));
  }

protected:
  SlotList m_slots;
};

// Signal1<R, A1>
template<typename R, typename A1>
class Signal1 : public Signal1_base<R, A1>
{
public:
  // GCC needs redefinitions
  typedef Signal1_base<R, A1> Base;
  typedef Slot1<R, A1> SlotType;
  typedef typename Base::SlotList SlotList;

  Signal1() { }

  R operator()(A1 a1, R default_result = R()) {
    R result(default_result);
    SlotList copy = Base::m_slots;
    for (typename SlotList::iterator it = copy.begin(), end = copy.end(); it != end; ++it) {
      SlotType* slot = *it;
      result = (*slot)(a1);
    }
    return result;
  }

  template<typename Merger>
  R operator()(A1 a1, R default_result, const Merger& m) {
    R result(default_result);
    Merger merger(m);
    SlotList copy = Base::m_slots;
    for (typename SlotList::iterator it = copy.begin(), end = copy.end(); it != end; ++it) {
      SlotType* slot = *it;
      result = merger(result, (*slot)(a1));
    }
    return result;
  }

};

// Signal1<void, A1>
template<typename A1>
class Signal1<void, A1> : public Signal1_base<void, A1>
{
public:
  // GCC needs redefinitions
  typedef Signal1_base<void, A1> Base;
  typedef Slot1<void, A1> SlotType;
  typedef typename Base::SlotList SlotList;

  Signal1() { }

  void operator()(A1 a1) {
    SlotList copy = Base::m_slots;
    for (typename SlotList::iterator it = copy.begin(), end = copy.end(); it != end; ++it) {
      SlotType* slot = *it;
      (*slot)(a1);
    }
  }

};

// Signal2_base<R, A1, A2> - Base class to delegate responsibility to
// functions of two arguments.
template<typename R, typename A1, typename A2>
class Signal2_base : public Signal {
public:
  typedef R ReturnType;
  typedef Slot2<R, A1, A2> SlotType;
  typedef std::vector<SlotType*> SlotList;

  Signal2_base() { }
  ~Signal2_base() {
    for (SlotType* slot : m_slots)
      delete slot;
  }

  Signal2_base(const Signal2_base&) { }
  Signal2_base operator=(const Signal2_base&) {
    return *this;
  }

  Connection addSlot(SlotType* slot) {
    m_slots.push_back(slot);
    return Connection(this, slot);
  }

  template<typename F>
  Connection connect(const F& f) {
    return addSlot(new Slot2_fun<R, F, A1, A2>(f));
  }

  template<class T>
  Connection connect(R (T::*m)(A1, A2), T* t) {
    return addSlot(new Slot2_mem<R, T, A1, A2>(m, t));
  }

  virtual void disconnectSlot(Slot* slot) override {
    base::remove_from_container(m_slots, static_cast<SlotType*>(slot));
  }

protected:
  SlotList m_slots;
};

// Signal2<R, A1>
template<typename R, typename A1, typename A2>
class Signal2 : public Signal2_base<R, A1, A2> {
public:
  // GCC needs redefinitions
  typedef Signal2_base<R, A1, A2> Base;
  typedef Slot2<R, A1, A2> SlotType;
  typedef typename Base::SlotList SlotList;

  Signal2() {
  }

  R operator()(A1 a1, A2 a2, R default_result = R()) {
    R result(default_result);
    SlotList copy = Base::m_slots;
    for (typename SlotList::iterator it = copy.begin(), end = copy.end(); it != end; ++it) {
      SlotType* slot = *it;
      result = (*slot)(a1, a2);
    }
    return result;
  }

  template<typename Merger>
  R operator()(A1 a1, A2 a2, R default_result, const Merger& m) {
    R result(default_result);
    Merger merger(m);
    SlotList copy = Base::m_slots;
    for (typename SlotList::iterator it = copy.begin(), end = copy.end(); it != end; ++it) {
      SlotType* slot = *it;
      result = merger(result, (*slot)(a1, a2));
    }
    return result;
  }

};

// Signal2<void, A1>
template<typename A1, typename A2>
class Signal2<void, A1, A2> : public Signal2_base<void, A1, A2> {
public:
  // GCC needs redefinitions
  typedef Signal2_base<void, A1, A2> Base;
  typedef Slot2<void, A1, A2> SlotType;
  typedef typename Base::SlotList SlotList;

  Signal2() {
  }

  void operator()(A1 a1, A2 a2) {
    SlotList copy = Base::m_slots;
    for (typename SlotList::iterator it = copy.begin(), end = copy.end(); it != end; ++it) {
      SlotType* slot = *it;
      (*slot)(a1, a2);
    }
  }

};

} // namespace base

#endif
