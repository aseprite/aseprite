// Aseprite
// Copyright (C) 2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_LAYOUT_H_INCLUDED
#define APP_UI_LAYOUT_H_INCLUDED
#pragma once

#include <memory>
#include <string>

namespace app {
class Dock;

class Layout {
public:
  Layout(const std::string& name, const Dock* dock);

  const std::string& name() const { return m_name; }
  const std::string& data() const { return m_data; }

  bool loadLayout(Dock* dock) const;

private:
  std::string m_name;
  std::string m_data;
};

using LayoutPtr = std::shared_ptr<Layout>;

} // namespace app

#endif
