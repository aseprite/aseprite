// Aseprite Base Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_CONNETION_H_INCLUDED
#define BASE_CONNETION_H_INCLUDED
#pragma once

namespace base {

class Signal;
class Slot;

class Connection {
public:
  Connection() : m_signal(NULL), m_slot(NULL) {
  }

  Connection(Signal* signal, Slot* slot) :
    m_signal(signal), m_slot(slot) {
  }

  void disconnect();

  operator bool() {
    return (m_slot != NULL);
  }

private:
  Signal* m_signal;
  Slot* m_slot;
};

class ScopedConnection {
public:
  ScopedConnection() {
  }

  ScopedConnection(const Connection& conn) : m_conn(conn) {
  }

  ScopedConnection& operator=(const Connection& conn) {
    m_conn.disconnect();
    m_conn = conn;
    return *this;
  }

  ~ScopedConnection() {
    m_conn.disconnect();
  }

private:
  Connection m_conn;
};

} // namespace base

#endif
