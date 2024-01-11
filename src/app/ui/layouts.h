// Aseprite
// Copyright (c) 2022-2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_LAYOUTS_H_INCLUDED
#define APP_UI_LAYOUTS_H_INCLUDED
#pragma once

#include "app/ui/layout.h"

#include <string>
#include <vector>

namespace app {

class Layouts {
public:
  Layouts();
  ~Layouts();

  size_t size() const { return m_layouts.size(); }

  LayoutPtr getById(const std::string& id) const;

  // Returns true if the layout is added, or false if it was
  // replaced.
  bool addLayout(const LayoutPtr& layout);

  // To iterate layouts
  using List = std::vector<LayoutPtr>;
  using iterator = List::iterator;
  iterator begin() { return m_layouts.begin(); }
  iterator end() { return m_layouts.end(); }

private:
  void load(const std::string& fn);
  void save(const std::string& fn) const;
  static std::string UserLayoutsFilename();

  List m_layouts;
  std::string m_userLayoutsFilename;
};

} // namespace app

#endif
