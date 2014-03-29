// SHE library
// Copyright (C) 2012-2014  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef SHE_EVENT_H_INCLUDED
#define SHE_EVENT_H_INCLUDED
#pragma once

#include <string>
#include <vector>

namespace she {

  class Event {
  public:
    enum Type {
      None,
      DropFiles,
    };

    typedef std::vector<std::string> Files;

    Event() : m_type(None) { }

    int type() const { return m_type; }
    void setType(Type type) { m_type = type; }

    const Files& files() const { return m_files; }
    void setFiles(const Files& files) { m_files = files; }

  private:
    Type m_type;
    Files m_files;
  };

} // namespace she

#endif
