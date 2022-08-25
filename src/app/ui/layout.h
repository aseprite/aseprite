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

class TiXmlElement;

namespace app {
class Dock;

class Layout;
using LayoutPtr = std::shared_ptr<Layout>;

class Layout final {
public:
  static LayoutPtr MakeFromXmlElement(const TiXmlElement* layoutElem);
  static LayoutPtr MakeFromDock(const std::string& name, const Dock* dock);

  const std::string& name() const { return m_name; }
  const TiXmlElement* xmlElement() const { return m_elem.get(); }

  bool loadLayout(Dock* dock) const;

private:
  std::string m_name;
  std::unique_ptr<TiXmlElement> m_elem;
};

} // namespace app

#endif
