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

#ifndef VACA_BIND_H
#define VACA_BIND_H

namespace Vaca {

/**
   @defgroup bind_group Bind Classes and Functions
   @{
 */

// ======================================================================
// BindAdapter0_fun

/**
   @see @ref page_bind
*/
template<typename R, typename F>
class BindAdapter0_fun
{
  F f;
public:
  BindAdapter0_fun(const F& f) : f(f) { }

  R operator()() { return f(); }

  template<typename A1>
  R operator()(const A1& a1) { return f(); }

  template<typename A1, typename A2>
  R operator()(const A1& a1, const A2& a2) { return f(); }

  template<typename A1, typename A2, typename A3>
  R operator()(const A1& a1, const A2& a2, const A3& a3) { return f(); }

  template<typename A1, typename A2, typename A3, typename A4>
  R operator()(const A1& a1, const A2& a2, const A3& a3, const A4& a4) { return f(); }
};

/**
   @see @ref page_bind
*/
template<typename F>
class BindAdapter0_fun<void, F>
{
  F f;
public:
  BindAdapter0_fun(const F& f) : f(f) { }

  void operator()() { f(); }

  template<typename A1>
  void operator()(const A1& a1) { f(); }

  template<typename A1, typename A2>
  void operator()(const A1& a1, const A2& a2) { f(); }

  template<typename A1, typename A2, typename A3>
  void operator()(const A1& a1, const A2& a2, const A3& a3) { f(); }

  template<typename A1, typename A2, typename A3, typename A4>
  void operator()(const A1& a1, const A2& a2, const A3& a3, const A4& a4) { f(); }
};

/**
   @see @ref page_bind
*/
template<typename R, typename F>
BindAdapter0_fun<R, F>
Bind(const F& f)
{
  return BindAdapter0_fun<R, F>(f);
}

// ======================================================================
// BindAdapter0_mem

/**
   @see @ref page_bind
*/
template<typename R, typename T>
class BindAdapter0_mem
{
  R (T::*m)();
  T* t;
public:
  template<typename T2>
  BindAdapter0_mem(R (T::*m)(), T2* t) : m(m), t(t) { }

  R operator()() { return (t->*m)(); }

  template <typename A1>
  R operator()(const A1& a1) { return (t->*m)(); }

  template <typename A1, typename A2>
  R operator()(const A1& a1, const A2& a2) { return (t->*m)(); }

  template <typename A1, typename A2, typename A3>
  R operator()(const A1& a1, const A2& a2, const A3& a3) { return (t->*m)(); }

  template <typename A1, typename A2, typename A3, typename A4>
  R operator()(const A1& a1, const A2& a2, const A3& a3, const A4& a4) { return (t->*m)(); }
};

/**
   @see @ref page_bind
*/
template<typename T>
class BindAdapter0_mem<void, T>
{
  void (T::*m)();
  T* t;
public:
  template<typename T2>
  BindAdapter0_mem(void (T::*m)(), T2* t) : m(m), t(t) { }

  void operator()() { (t->*m)(); }

  template <typename A1>
  void operator()(const A1& a1) { (t->*m)(); }

  template <typename A1, typename A2>
  void operator()(const A1& a1, const A2& a2) { (t->*m)(); }

  template <typename A1, typename A2, typename A3>
  void operator()(const A1& a1, const A2& a2, const A3& a3) { (t->*m)(); }

  template <typename A1, typename A2, typename A3, typename A4>
  void operator()(const A1& a1, const A2& a2, const A3& a3, const A4& a4) { (t->*m)(); }
};

/**
   @see @ref page_bind
*/
template<typename R, typename T, typename T2>
BindAdapter0_mem<R, T>
Bind(R (T::*m)(), T2* t)
{
  return BindAdapter0_mem<R, T>(m, t);
}

// ======================================================================
// BindAdapter1_fun

/**
   @see @ref page_bind
*/
template<typename R, typename F,
	 typename X1>
class BindAdapter1_fun
{
  F f;
  X1 x1;
public:
  BindAdapter1_fun(const F& f, X1 x1) : f(f), x1(x1) { }

  R operator()() { return f(x1); }

  template<typename A1>
  R operator()(const A1& a1) { return f(x1); }

  template<typename A1, typename A2>
  R operator()(const A1& a1, const A2& a2) { return f(x1); }

  template<typename A1, typename A2, typename A3>
  R operator()(const A1& a1, const A2& a2, const A3& a3) { return f(x1); }

  template<typename A1, typename A2, typename A3, typename A4>
  R operator()(const A1& a1, const A2& a2, const A3& a3, const A4& a4) { return f(x1); }
};

/**
   @see @ref page_bind
*/
template<typename F,
	 typename X1>
class BindAdapter1_fun<void, F, X1>
{
  F f;
  X1 x1;
public:
  BindAdapter1_fun(const F& f, X1 x1) : f(f), x1(x1) { }

  void operator()() { f(x1); }

  template<typename A1>
  void operator()(A1& a1) { f(x1); }

  template<typename A1, typename A2>
  void operator()(A1& a1, A2& a2) { f(x1); }

  template<typename A1, typename A2, typename A3>
  void operator()(A1& a1, A2& a2, A3& a3) { f(x1); }

  template<typename A1, typename A2, typename A3, typename A4>
  void operator()(A1& a1, A2& a2, A3& a3, A4& a4) { f(x1); }
};

/**
   @see @ref page_bind
*/
template<typename R, typename F,
	 typename X1>
BindAdapter1_fun<R, F, X1>
Bind(const F& f, X1 x1)
{
  return BindAdapter1_fun<R, F, X1>(f, x1);
}

// ======================================================================
// BindAdapter1_mem

/**
   @see @ref page_bind
*/
template<typename R, typename T,
	 typename B1,
	 typename X1>
class BindAdapter1_mem
{
  R (T::*m)(B1);
  T* t;
  X1 x1;
public:
  template<typename T2>
  BindAdapter1_mem(R (T::*m)(B1), T2* t, X1 x1) : m(m), t(t), x1(x1) { }

  R operator()() { return (t->*m)(x1); }

  template <typename A1>
  R operator()(const A1& a1) { return (t->*m)(x1); }

  template <typename A1, typename A2>
  R operator()(const A1& a1, const A2& a2) { return (t->*m)(x1); }

  template <typename A1, typename A2, typename A3>
  R operator()(const A1& a1, const A2& a2, const A3& a3) { return (t->*m)(x1); }

  template <typename A1, typename A2, typename A3, typename A4>
  R operator()(const A1& a1, const A2& a2, const A3& a3, const A4& a4) { return (t->*m)(x1); }
};

/**
   @see @ref page_bind
*/
template<typename T,
	 typename B1,
	 typename X1>
class BindAdapter1_mem<void, T, B1, X1>
{
  void (T::*m)(B1);
  T* t;
  X1 x1;
public:
  template<typename T2>
  BindAdapter1_mem(void (T::*m)(B1), T2* t, X1 x1) : m(m), t(t), x1(x1) { }

  void operator()() { (t->*m)(x1); }

  template <typename A1>
  void operator()(const A1& a1) { (t->*m)(x1); }

  template <typename A1, typename A2>
  void operator()(const A1& a1, const A2& a2) { (t->*m)(x1); }

  template <typename A1, typename A2, typename A3>
  void operator()(const A1& a1, const A2& a2, const A3& a3) { (t->*m)(x1); }

  template <typename A1, typename A2, typename A3, typename A4>
  void operator()(const A1& a1, const A2& a2, const A3& a3, const A4& a4) { (t->*m)(x1); }
};

/**
   @see @ref page_bind
*/
template<typename R, typename T, typename T2,
	 typename B1, typename X1>
BindAdapter1_mem<R, T, B1, X1>
Bind(R (T::*m)(B1), T2* t, X1 x1)
{
  return BindAdapter1_mem<R, T, B1, X1>(m, t, x1);
}

// ======================================================================
// BindAdapter2_fun

/**
   @see @ref page_bind
*/
template<typename R, typename F,
	 typename X1, typename X2>
class BindAdapter2_fun
{
  F f;
  X1 x1;
  X2 x2;
public:
  BindAdapter2_fun(const F& f, X1 x1, X2 x2) : f(f), x1(x1), x2(x2) { }

  R operator()() { return f(x1, x2); }

  template<typename A1>
  R operator()(const A1& a1) { return f(x1, x2); }

  template<typename A1, typename A2>
  R operator()(const A1& a1, const A2& a2) { return f(x1, x2); }

  template<typename A1, typename A2, typename A3>
  R operator()(const A1& a1, const A2& a2, const A3& a3) { return f(x1, x2); }

  template<typename A1, typename A2, typename A3, typename A4>
  R operator()(const A1& a1, const A2& a2, const A3& a3, const A4& a4) { return f(x1, x2); }
};

/**
   @see @ref page_bind
*/
template<typename F,
	 typename X1, typename X2>
class BindAdapter2_fun<void, F, X1, X2>
{
  F f;
  X1 x1;
  X2 x2;
public:
  BindAdapter2_fun(const F& f, X1 x1, X2 x2) : f(f), x1(x1), x2(x2) { }

  void operator()() { f(x1, x2); }

  template<typename A1>
  void operator()(const A1& a1) { f(x1, x2); }

  template<typename A1, typename A2>
  void operator()(const A1& a1, const A2& a2) { f(x1, x2); }

  template<typename A1, typename A2, typename A3>
  void operator()(const A1& a1, const A2& a2, const A3& a3) { f(x1, x2); }

  template<typename A1, typename A2, typename A3, typename A4>
  void operator()(const A1& a1, const A2& a2, const A3& a3, const A4& a4) { f(x1, x2); }
};

/**
   @see @ref page_bind
*/
template<typename R, typename F,
	 typename X1, typename X2>
BindAdapter2_fun<R, F, X1, X2>
Bind(const F& f, X1 x1, X2 x2)
{
  return BindAdapter2_fun<R, F, X1, X2>(f, x1, x2);
}

// ======================================================================
// BindAdapter2_mem

/**
   @see @ref page_bind
*/
template<typename R, typename T,
	 typename B1, typename B2,
	 typename X1, typename X2>
class BindAdapter2_mem
{
  R (T::*m)(B1, B2);
  T* t;
  X1 x1;
  X2 x2;
public:
  template<typename T2>
  BindAdapter2_mem(R (T::*m)(B1, B2), T2* t, X1 x1, X2 x2) : m(m), t(t), x1(x1), x2(x2) { }

  R operator()() { return (t->*m)(x1, x2); }

  template<typename A1>
  R operator()(const A1& a1) { return (t->*m)(x1, x2); }

  template<typename A1, typename A2>
  R operator()(const A1& a1, const A2& a2) { return (t->*m)(x1, x2); }

  template<typename A1, typename A2, typename A3>
  R operator()(const A1& a1, const A2& a2, const A3& a3) { return (t->*m)(x1, x2); }

  template<typename A1, typename A2, typename A3, typename A4>
  R operator()(const A1& a1, const A2& a2, const A3& a3, const A4& a4) { return (t->*m)(x1, x2); }
};

/**
   @see @ref page_bind
*/
template<typename T,
	 typename B1, typename B2,
	 typename X1, typename X2>
class BindAdapter2_mem<void, T, B1, B2, X1, X2>
{
  void (T::*m)(B1, B2);
  T* t;
  X1 x1;
  X2 x2;
public:
  template<typename T2>
  BindAdapter2_mem(void (T::*m)(B1, B2), T2* t, X1 x1, X2 x2) : m(m), t(t), x1(x1), x2(x2) { }

  void operator()() { (t->*m)(x1, x2); }

  template<typename A1>
  void operator()(const A1& a1) { (t->*m)(x1, x2); }

  template<typename A1, typename A2>
  void operator()(const A1& a1, const A2& a2) { (t->*m)(x1, x2); }

  template<typename A1, typename A2, typename A3>
  void operator()(const A1& a1, const A2& a2, const A3& a3) { (t->*m)(x1, x2); }

  template<typename A1, typename A2, typename A3, typename A4>
  void operator()(const A1& a1, const A2& a2, const A3& a3, const A4& a4) { (t->*m)(x1, x2); }
};

/**
   @see @ref page_bind
*/
template<typename R, typename T, typename T2, typename B1, typename B2, typename X1, typename X2>
BindAdapter2_mem<R, T, B1, B2, X1, X2>
Bind(R (T::*m)(B1, B2), T2* t, X1 x1, X2 x2)
{
  return BindAdapter2_mem<R, T, B1, B2, X1, X2>(m, t, x1, x2);
}

// ======================================================================
// BindAdapter3_fun

/**
   @see @ref page_bind
*/
template<typename R, typename F,
	 typename X1, typename X2, typename X3>
class BindAdapter3_fun
{
  F f;
  X1 x1;
  X2 x2;
  X3 x3;
public:
  BindAdapter3_fun(const F& f, X1 x1, X2 x2, X3 x3) : f(f), x1(x1), x2(x2), x3(x3) { }

  R operator()() { return f(x1, x2, x3); }

  template<typename A1>
  R operator()(const A1& a1) { return f(x1, x2, x3); }

  template<typename A1, typename A2>
  R operator()(const A1& a1, const A2& a2) { return f(x1, x2, x3); }

  template<typename A1, typename A2, typename A3>
  R operator()(const A1& a1, const A2& a2, const A3& a3) { return f(x1, x2, x3); }

  template<typename A1, typename A2, typename A3, typename A4>
  R operator()(const A1& a1, const A2& a2, const A3& a3, const A4& a4) { return f(x1, x2, x3); }
};

/**
   @see @ref page_bind
*/
template<typename F,
	 typename X1, typename X2, typename X3>
class BindAdapter3_fun<void, F, X1, X2, X3>
{
  F f;
  X1 x1;
  X2 x2;
  X3 x3;
public:
  BindAdapter3_fun(const F& f, X1 x1, X2 x2, X3 x3) : f(f), x1(x1), x2(x2), x3(x3) { }

  void operator()() { f(x1, x2, x3); }

  template<typename A1>
  void operator()(const A1& a1) { f(x1, x2, x3); }

  template<typename A1, typename A2>
  void operator()(const A1& a1, const A2& a2) { f(x1, x2, x3); }

  template<typename A1, typename A2, typename A3>
  void operator()(const A1& a1, const A2& a2, const A3& a3) { f(x1, x2, x3); }

  template<typename A1, typename A2, typename A3, typename A4>
  void operator()(const A1& a1, const A2& a2, const A3& a3, const A4& a4) { f(x1, x2, x3); }
};

/**
   @see @ref page_bind
*/
template<typename R, typename F,
	 typename X1, typename X2, typename X3>
BindAdapter3_fun<R, F, X1, X2, X3>
Bind(const F& f, X1 x1, X2 x2, X3 x3)
{
  return BindAdapter3_fun<R, F, X1, X2, X3>(f, x1, x2, x3);
}

// ======================================================================
// BindAdapter3_mem

/**
   @see @ref page_bind
*/
template<typename R, typename T,
	 typename B1, typename B2, typename B3,
	   typename X1, typename X2, typename X3>
class BindAdapter3_mem
{
  R (T::*m)(B1, B2, B3);
  T* t;
  X1 x1;
  X2 x2;
  X3 x3;
public:
  template<typename T2>
  BindAdapter3_mem(R (T::*m)(B1, B2, B3), T2* t, X1 x1, X2 x2, X3 x3) : m(m), t(t), x1(x1), x2(x2), x3(x3) { }

  R operator()() { return (t->*m)(x1, x2, x3); }

  template<typename A1>
  R operator()(const A1& a1) { return (t->*m)(x1, x2, x3); }

  template<typename A1, typename A2>
  R operator()(const A1& a1, const A2& a2) { return (t->*m)(x1, x2, x3); }

  template<typename A1, typename A2, typename A3>
  R operator()(const A1& a1, const A2& a2, const A3& a3) { return (t->*m)(x1, x2, x3); }

  template<typename A1, typename A2, typename A3, typename A4>
  R operator()(const A1& a1, const A2& a2, const A3& a3, const A4& a4) { return (t->*m)(x1, x2, x3); }
};

/**
   @see @ref page_bind
*/
template<typename T,
	 typename B1, typename B2, typename B3,
	 typename X1, typename X2, typename X3>
class BindAdapter3_mem<void, T, B1, B2, B3, X1, X2, X3>
{
  void (T::*m)(B1, B2, B3);
  T* t;
  X1 x1;
  X2 x2;
  X3 x3;
public:
  template<typename T2>
  BindAdapter3_mem(void (T::*m)(B1, B2, B3), T2* t, X1 x1, X2 x2) : m(m), t(t), x1(x1), x2(x2) { }

  void operator()() { (t->*m)(x1, x2, x3); }

  template<typename A1>
  void operator()(const A1& a1) { (t->*m)(x1, x2, x3); }

  template<typename A1, typename A2>
  void operator()(const A1& a1, const A2& a2) { (t->*m)(x1, x2, x3); }

  template<typename A1, typename A2, typename A3>
  void operator()(const A1& a1, const A2& a2, const A3& a3) { (t->*m)(x1, x2, x3); }

  template<typename A1, typename A2, typename A3, typename A4>
  void operator()(const A1& a1, const A2& a2, const A3& a3, const A4& a4) { (t->*m)(x1, x2, x3); }
};

/**
   @see @ref page_bind
*/
template<typename R, typename T, typename T2,
	 typename B1, typename B2, typename B3,
	 typename X1, typename X2, typename X3>
BindAdapter3_mem<R, T, B1, B2, B3, X1, X2, X3>
Bind(R (T::*m)(B1, B2, B3), T2* t, X1 x1, X2 x2)
{
  return BindAdapter3_mem<R, T, B1, B2, B3, X1, X2, X3>(m, t, x1, x2);
}

// ======================================================================
// RefWrapper

/**
   @todo

   @see @ref page_bind
*/
template<class T>
class RefWrapper
{
  T* ptr;
public:
  RefWrapper(T& ref) : ptr(&ref) { }
  operator T&() const { return *ptr; }
};

/**
   Creates RefWrappers, useful to wrap arguments that have to be
   passed as a reference when you use Bind.

   @see @ref page_bind
*/
template<class T>
RefWrapper<T> Ref(T& ref)
{
  return RefWrapper<T>(ref);
}

/** @} */

} // namespace Vaca

#endif // VACA_BIND_H
