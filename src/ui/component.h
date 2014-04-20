// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_COMPONENT_H_INCLUDED
#define UI_COMPONENT_H_INCLUDED
#pragma once

#include <map>

#include "base/disable_copying.h"
#include "base/string.h"
#include "ui/property.h"

namespace ui {

  // A component is a visual object, such as widgets or menus.
  //
  // Components are non-copyable.
  class Component
  {
  public:
    typedef std::map<base::string, PropertyPtr> Properties;

    Component();
    virtual ~Component();

    PropertyPtr getProperty(const base::string& name);
    void setProperty(PropertyPtr property);

    bool hasProperty(const base::string& name);
    void removeProperty(const base::string& name);

    const Properties& getProperties() const;

  private:
    Properties m_properties;

    DISABLE_COPYING(Component);
  };

} // namespace ui

#endif
