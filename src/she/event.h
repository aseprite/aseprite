// SHE library
// Copyright (C) 2012-2014  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef SHE_EVENT_H_INCLUDED
#define SHE_EVENT_H_INCLUDED

namespace she {

  class Event {
  public:
    enum Type {
      None,
    };

    Event() : m_type(None) { }

    int type() const { return m_type; }
    void setType(Type type) { m_type = type; }

  private:
    Type m_type;
  };

} // namespace she

#endif
