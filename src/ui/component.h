// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_COMPONENT_H_INCLUDED
#define UI_COMPONENT_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "ui/property.h"

#include <map>
#include <string>

namespace ui {

// A component is a visual object, such as widgets or menus.
//
// Components are non-copyable.
class Component {
public:
  typedef std::map<std::string, PropertyPtr> Properties;

  Component();
  virtual ~Component();

  PropertyPtr getProperty(const std::string& name) const;
  void setProperty(PropertyPtr property);

  bool hasProperty(const std::string& name) const;
  void removeProperty(const std::string& name);

  const Properties& getProperties() const;

private:
  Properties m_properties;

  DISABLE_COPYING(Component);
};

} // namespace ui

#endif
