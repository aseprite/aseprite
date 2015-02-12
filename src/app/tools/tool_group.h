// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_TOOLS_TOOL_GROUP_H_INCLUDED
#define APP_TOOLS_TOOL_GROUP_H_INCLUDED
#pragma once

#include <string>

namespace app {
  namespace tools {

    // A group of tools.
    class ToolGroup {
    public:
      ToolGroup(const char* name,
                const char* label) : m_name(name)
                                   , m_label(label) { }

      const std::string& getName() const { return m_name; }
      const std::string& getLabel() const { return m_label; }

    private:
      std::string m_name;
      std::string m_label;
    };

  } // namespace tools
} // namespace app

#endif  // TOOLS_TOOL_GROUP_H_INCLUDED
