// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TOOLS_TOOL_GROUP_H_INCLUDED
#define APP_TOOLS_TOOL_GROUP_H_INCLUDED
#pragma once

#include <string>

namespace app { namespace tools {

// A group of tools.
class ToolGroup {
public:
  ToolGroup(const char* id) : m_id(id) {}

  const std::string& id() const { return m_id; }

  bool isVisible() const { return m_visible; }
  void setVisible(bool visible) { m_visible = visible; }

private:
  std::string m_id;
  bool m_visible = true;
};

}} // namespace app::tools

#endif // TOOLS_TOOL_GROUP_H_INCLUDED
