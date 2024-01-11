// Aseprite
// Copyright (C) 2022-2024  Igara Studio S.A.
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
  static constexpr const char* kDefault = "_default_";
  static constexpr const char* kMirroredDefault = "_mirrored_default_";

  static LayoutPtr MakeFromXmlElement(const TiXmlElement* layoutElem);
  static LayoutPtr MakeFromDock(const std::string& id, const std::string& name, const Dock* dock);

  const std::string& id() const { return m_id; }
  const std::string& name() const { return m_name; }
  const TiXmlElement* xmlElement() const { return m_elem.get(); }

  bool matchId(const std::string& id) const;
  bool loadLayout(Dock* dock) const;

private:
  std::string m_id;
  std::string m_name;
  std::unique_ptr<TiXmlElement> m_elem;
};

} // namespace app

#endif
