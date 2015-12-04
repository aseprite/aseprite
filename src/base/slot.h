// Aseprite Base Library
// Copyright (c) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_SLOT_H_INCLUDED
#define BASE_SLOT_H_INCLUDED
#pragma once

#include "base/disable_copying.h"

namespace base {

class Slot {
public:
  Slot() { }
  virtual ~Slot() { }

private:
  DISABLE_COPYING(Slot);
};

// Slot0 - Base class for delegates of zero arguments.
template<typename R>
class Slot0 : public Slot {
public:
  Slot0() { }
  Slot0(const Slot0& s) { (void)s; }
  virtual ~Slot0() { }
  virtual R operator()() = 0;
};

// Slot0_fun - hold a F instance and use the function call operator
template<typename R, typename F>
class Slot0_fun : public Slot0<R> {
  F f;
public:
  Slot0_fun(const F& f) : f(f) { }
  Slot0_fun(const Slot0_fun& s) : Slot0<R>(s), f(s.f) { }
  ~Slot0_fun() { }
  R operator()() { return f(); }
};

template<typename F>
class Slot0_fun<void, F> : public Slot0<void> {
  F f;
public:
  Slot0_fun(const F& f) : f(f) { }
  Slot0_fun(const Slot0_fun& s) : Slot0<void>(s), f(s.f) { }
  ~Slot0_fun() { }
  void operator()() { f(); }
};

// Slot0_mem - pointer to a member function of the T class
template<typename R, class T>
class Slot0_mem : public Slot0<R> {
  R (T::*m)();
  T* t;
public:
  Slot0_mem(R (T::*m)(), T* t) : m(m), t(t) { }
  Slot0_mem(const Slot0_mem& s) : Slot0<R>(s), m(s.m), t(s.t) { }
  ~Slot0_mem() { }
  R operator()() { return (t->*m)(); }
};

template<class T>
class Slot0_mem<void, T> : public Slot0<void> {
  void (T::*m)();
  T* t;
public:
  Slot0_mem(void (T::*m)(), T* t) : m(m), t(t) { }
  Slot0_mem(const Slot0_mem& s) : Slot0<void>(s), m(s.m), t(s.t) { }
  ~Slot0_mem() { }
  void operator()() { (t->*m)(); }
};

// Slot1 - Base class for delegates of one argument.
template<typename R, typename A1>
class Slot1 : public Slot {
public:
  Slot1() { }
  Slot1(const Slot1& s) { (void)s; }
  virtual ~Slot1() { }
  virtual R operator()(A1 a1) = 0;
};

// Slot1_fun - hold a F instance and use the function call operator
template<typename R, typename F, typename A1>
class Slot1_fun : public Slot1<R, A1> {
  F f;
public:
  Slot1_fun(const F& f) : f(f) { }
  Slot1_fun(const Slot1_fun& s) : Slot1<R, A1>(s), f(s.f) { }
  ~Slot1_fun() { }
  R operator()(A1 a1) { return f(a1); }
};

template<typename F, typename A1>
class Slot1_fun<void, F, A1> : public Slot1<void, A1> {
  F f;
public:
  Slot1_fun(const F& f) : f(f) { }
  Slot1_fun(const Slot1_fun& s) : Slot1<void, A1>(s), f(s.f) { }
  ~Slot1_fun() { }
  void operator()(A1 a1) { f(a1); }
};

// Slot1_mem - pointer to a member function of the T class
template<typename R, class T, typename A1>
class Slot1_mem : public Slot1<R, A1> {
  R (T::*m)(A1);
  T* t;
public:
  Slot1_mem(R (T::*m)(A1), T* t) : m(m), t(t) { }
  Slot1_mem(const Slot1_mem& s) : Slot1<R, A1>(s), m(s.m), t(s.t) { }
  ~Slot1_mem() { }
  R operator()(A1 a1) { return (t->*m)(a1); }
};

template<class T, typename A1>
class Slot1_mem<void, T, A1> : public Slot1<void, A1> {
  void (T::*m)(A1);
  T* t;
public:
  Slot1_mem(void (T::*m)(A1), T* t) : m(m), t(t) { }
  Slot1_mem(const Slot1_mem& s) : Slot1<void, A1>(s), m(s.m), t(s.t) { }
  ~Slot1_mem() { }
  void operator()(A1 a1) { (t->*m)(a1); }
};

// Slot2 - Base class for delegates of two arguments.
template<typename R, typename A1, typename A2>
class Slot2 : public Slot {
public:
  Slot2() { }
  Slot2(const Slot2& s) { (void)s; }
  virtual ~Slot2() { }
  virtual R operator()(A1 a1, A2 a2) = 0;
};

// Slot2_fun - hold a F instance and use the function call operator
template<typename R, typename F, typename A1, typename A2>
class Slot2_fun : public Slot2<R, A1, A2> {
  F f;
public:
  Slot2_fun(const F& f) : f(f) { }
  Slot2_fun(const Slot2_fun& s) : Slot2<R, A1, A2>(s), f(s.f) { }
  ~Slot2_fun() { }
  R operator()(A1 a1, A2 a2) { return f(a1, a2); }
};

template<typename F, typename A1, typename A2>
class Slot2_fun<void, F, A1, A2> : public Slot2<void, A1, A2> {
  F f;
public:
  Slot2_fun(const F& f) : f(f) { }
  Slot2_fun(const Slot2_fun& s) : Slot2<void, A1, A2>(s), f(s.f) { }
  ~Slot2_fun() { }
  void operator()(A1 a1, A2 a2) { f(a1, a2); }
};

// Slot2_mem - pointer to a member function of the T class
template<typename R, class T, typename A1, typename A2>
class Slot2_mem : public Slot2<R, A1, A2> {
  R (T::*m)(A1, A2);
  T* t;
public:
  Slot2_mem(R (T::*m)(A1, A2), T* t) : m(m), t(t) { }
  Slot2_mem(const Slot2_mem& s) : Slot2<R, A1, A2>(s), m(s.m), t(s.t) { }
  ~Slot2_mem() { }
  R operator()(A1 a1, A2 a2) { return (t->*m)(a1, a2); }
};

template<class T, typename A1, typename A2>
class Slot2_mem<void, T, A1, A2> : public Slot2<void, A1, A2> {
  void (T::*m)(A1, A2);
  T* t;
public:
  Slot2_mem(void (T::*m)(A1, A2), T* t) : m(m), t(t) { }
  Slot2_mem(const Slot2_mem& s) : Slot2<void, A1, A2>(s), m(s.m), t(s.t) { }
  ~Slot2_mem() { }
  void operator()(A1 a1, A2 a2) { return (t->*m)(a1, a2); }
};

} // namespace base

#endif
