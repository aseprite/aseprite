// ASE base library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_THREAD_H_INCLUDED
#define GUI_THREAD_H_INCLUDED

namespace base {		// Based on C++0x threads lib

  class thread {
  public:
    class details;
    class id
    {
      friend class thread;
      friend class details;

      unsigned int m_native_id;
      id(unsigned int id) : m_native_id(id) { }
    public:
      id() : m_native_id(0) { }
      bool operator==(const id& y) const { return m_native_id == y.m_native_id; }
      bool operator!=(const id& y) const { return m_native_id != y.m_native_id; }
      bool operator< (const id& y) const { return m_native_id <  y.m_native_id; }
      bool operator<=(const id& y) const { return m_native_id <= y.m_native_id; }
      bool operator> (const id& y) const { return m_native_id >  y.m_native_id; }
      bool operator>=(const id& y) const { return m_native_id >= y.m_native_id; }

      id& operator=(const id& y) { m_native_id = y.m_native_id; }
    };

    typedef void* native_handle_type;

  public:

    // Create an instance to represent the current thread
    thread();

    // Create a new thread without arguments
    template<class Callable>
    thread(const Callable& f) {
      launch_thread(new func_wrapper0<Callable>(f));
    }

    // Create a new thread with one argument
    template<class Callable, class A>
    thread(const Callable& f, A a) {
      launch_thread(new func_wrapper1<Callable, A>(f, a));
    }

    // Create a new thread with two arguments
    template<class Callable, class A, class B>
    thread(const Callable& f, A a, B b) {
      launch_thread(new func_wrapper2<Callable, A, B>(f, a, b));
    }

    ~thread();

    bool joinable() const;
    void join();
    void detach();

    id get_id() const;
    native_handle_type native_handle();

    class details {
    public:
      static void thread_proxy(void* data);
      static id get_current_thread_id();
    };

  private:
    native_handle_type m_native_handle;
    id m_id;

    class func_wrapper {
    public:
      virtual void operator()() = 0;
    };

    void launch_thread(func_wrapper* f);

    template<class Callable>
    class func_wrapper0 : public func_wrapper {
    public:
      Callable f;
      func_wrapper0(const Callable& f) : f(f) { }
      void operator()() { f(); }
    };

    template<class Callable, class A>
    class func_wrapper1 : public func_wrapper {
    public:
      Callable f;
      A a;
      func_wrapper1(const Callable& f, A a) : f(f), a(a) { }
      void operator()() { f(a); }
    };

    template<class Callable, class A, class B>
    class func_wrapper2 : public func_wrapper {
    public:
      Callable f;
      A a;
      B b;
      func_wrapper2(const Callable& f, A a, B b) : f(f), a(a), b(b) { }
      void operator()() { f(a, b); }
    };

  };

  namespace this_thread
  {
    thread::id get_id();
    void yield();
    void sleep_for(int milliseconds);
  }

  // This class joins the thread in its destructor. 
  class thread_guard {
    thread& m_thread;
  public:
    explicit thread_guard(thread& t) : m_thread(t) { }
    ~thread_guard()
    {
      if (m_thread.joinable())
	m_thread.join();
    }
  };

}

#endif
