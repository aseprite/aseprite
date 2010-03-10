// Vaca - Visual Application Components Abstraction
// Copyright (c) 2005-2009 David Capello
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in
//   the documentation and/or other materials provided with the
//   distribution.
// * Neither the name of the author nor the names of its contributors
//   may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef VACA_SLOT_H
#define VACA_SLOT_H

#include "Vaca/base.h"

namespace Vaca {

/**
   @defgroup slot_group Slot Classes
   @{
 */

// ======================================================================
// Slot0

template<typename R>
class Slot0
{
public:
  Slot0() { }
  Slot0(const Slot0& s) { (void)s; }
  virtual ~Slot0() { }
  virtual R operator()() = 0;
  virtual Slot0* clone() const = 0;
};

// ======================================================================
// Slot0_fun - hold a F instance and use the function call operator

template<typename R, typename F>
class Slot0_fun : public Slot0<R>
{
  F f;
public:
  Slot0_fun(const F& f) : f(f) { }
  Slot0_fun(const Slot0_fun& s) : Slot0<R>(s), f(s.f) { }
  ~Slot0_fun() { }
  R operator()() { return f(); }
  Slot0_fun* clone() const { return new Slot0_fun(*this); }
};

template<typename F>
class Slot0_fun<void, F> : public Slot0<void>
{
  F f;
public:
  Slot0_fun(const F& f) : f(f) { }
  Slot0_fun(const Slot0_fun& s) : Slot0<void>(s), f(s.f) { }
  ~Slot0_fun() { }
  void operator()() { f(); }
  Slot0_fun* clone() const { return new Slot0_fun(*this); }
};
  
// ======================================================================
// Slot0_mem - pointer to a member function of the T class

template<typename R, class T>
class Slot0_mem : public Slot0<R>
{
  R (T::*m)();
  T* t;
public:
  Slot0_mem(R (T::*m)(), T* t) : m(m), t(t) { }
  Slot0_mem(const Slot0_mem& s) : Slot0<R>(s), m(s.m), t(s.t) { }
  ~Slot0_mem() { }
  R operator()() { return (t->*m)(); }
  Slot0_mem* clone() const { return new Slot0_mem(*this); }
};

template<class T>
class Slot0_mem<void, T> : public Slot0<void>
{
  void (T::*m)();
  T* t;
public:
  Slot0_mem(void (T::*m)(), T* t) : m(m), t(t) { }
  Slot0_mem(const Slot0_mem& s) : Slot0<void>(s), m(s.m), t(s.t) { }
  ~Slot0_mem() { }
  void operator()() { (t->*m)(); }
  Slot0_mem* clone() const { return new Slot0_mem(*this); }
};

// ======================================================================
// Slot1

template<typename R, typename A1>
class Slot1
{
public:
  Slot1() { }
  Slot1(const Slot1& s) { (void)s; }
  virtual ~Slot1() { }
  virtual R operator()(A1 a1) = 0;
  virtual Slot1* clone() const = 0;
};

// ======================================================================
// Slot1_fun - hold a F instance and use the function call operator

template<typename R, typename F, typename A1>
class Slot1_fun : public Slot1<R, A1>
{
  F f;
public:
  Slot1_fun(const F& f) : f(f) { }
  Slot1_fun(const Slot1_fun& s) : Slot1<R, A1>(s), f(s.f) { }
  ~Slot1_fun() { }
  R operator()(A1 a1) { return f(a1); }
  Slot1_fun* clone() const { return new Slot1_fun(*this); }
};

template<typename F, typename A1>
class Slot1_fun<void, F, A1> : public Slot1<void, A1>
{
  F f;
public:
  Slot1_fun(const F& f) : f(f) { }
  Slot1_fun(const Slot1_fun& s) : Slot1<void, A1>(s), f(s.f) { }
  ~Slot1_fun() { }
  void operator()(A1 a1) { f(a1); }
  Slot1_fun* clone() const { return new Slot1_fun(*this); }
};
  
// ======================================================================
// Slot1_mem - pointer to a member function of the T class

template<typename R, class T, typename A1>
class Slot1_mem : public Slot1<R, A1>
{
  R (T::*m)(A1);
  T* t;
public:
  Slot1_mem(R (T::*m)(A1), T* t) : m(m), t(t) { }
  Slot1_mem(const Slot1_mem& s) : Slot1<R, A1>(s), m(s.m), t(s.t) { }
  ~Slot1_mem() { }
  R operator()(A1 a1) { return (t->*m)(a1); }
  Slot1_mem* clone() const { return new Slot1_mem(*this); }
};

template<class T, typename A1>
class Slot1_mem<void, T, A1> : public Slot1<void, A1>
{
  void (T::*m)(A1);
  T* t;
public:
  Slot1_mem(void (T::*m)(A1), T* t) : m(m), t(t) { }
  Slot1_mem(const Slot1_mem& s) : Slot1<void, A1>(s), m(s.m), t(s.t) { }
  ~Slot1_mem() { }
  void operator()(A1 a1) { (t->*m)(a1); }
  Slot1_mem* clone() const { return new Slot1_mem(*this); }
};

// ======================================================================
// Slot2

template<typename R, typename A1, typename A2>
class Slot2
{
public:
  Slot2() { }
  Slot2(const Slot2& s) { (void)s; }
  virtual ~Slot2() { }
  virtual R operator()(A1 a1, A2 a2) = 0;
  virtual Slot2* clone() const = 0;
};

// ======================================================================
// Slot2_fun - hold a F instance and use the function call operator

template<typename R, typename F, typename A1, typename A2>
class Slot2_fun : public Slot2<R, A1, A2>
{
  F f;
public:
  Slot2_fun(const F& f) : f(f) { }
  Slot2_fun(const Slot2_fun& s) : Slot2<R, A1, A2>(s), f(s.f) { }
  ~Slot2_fun() { }
  R operator()(A1 a1, A2 a2) { return f(a1, a2); }
  Slot2_fun* clone() const { return new Slot2_fun(*this); }
};

template<typename F, typename A1, typename A2>
class Slot2_fun<void, F, A1, A2> : public Slot2<void, A1, A2>
{
  F f;
public:
  Slot2_fun(const F& f) : f(f) { }
  Slot2_fun(const Slot2_fun& s) : Slot2<void, A1, A2>(s), f(s.f) { }
  ~Slot2_fun() { }
  void operator()(A1 a1, A2 a2) { f(a1, a2); }
  Slot2_fun* clone() const { return new Slot2_fun(*this); }
};

// ======================================================================
// Slot2_mem - pointer to a member function of the T class

template<typename R, class T, typename A1, typename A2>
class Slot2_mem : public Slot2<R, A1, A2>
{
  R (T::*m)(A1, A2);
  T* t;
public:
  Slot2_mem(R (T::*m)(A1, A2), T* t) : m(m), t(t) { }
  Slot2_mem(const Slot2_mem& s) : Slot2<R, A1, A2>(s), m(s.m), t(s.t) { }
  ~Slot2_mem() { }
  R operator()(A1 a1, A2 a2) { return (t->*m)(a1, a2); }
  Slot2_mem* clone() const { return new Slot2_mem(*this); }
};

template<class T, typename A1, typename A2>
class Slot2_mem<void, T, A1, A2> : public Slot2<void, A1, A2>
{
  void (T::*m)(A1, A2);
  T* t;
public:
  Slot2_mem(void (T::*m)(A1, A2), T* t) : m(m), t(t) { }
  Slot2_mem(const Slot2_mem& s) : Slot2<void, A1, A2>(s), m(s.m), t(s.t) { }
  ~Slot2_mem() { }
  void operator()(A1 a1, A2 a2) { return (t->*m)(a1, a2); }
  Slot2_mem* clone() const { return new Slot2_mem(*this); }
};

// ======================================================================

/** @} */

} // namespace Vaca

#endif // VACA_SLOT_H
