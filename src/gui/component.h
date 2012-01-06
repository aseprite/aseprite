// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_COMPONENT_H_INCLUDED
#define GUI_COMPONENT_H_INCLUDED

#include <map>

#include "base/string.h"
#include "base/disable_copying.h"
#include "gui/property.h"

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

#endif
