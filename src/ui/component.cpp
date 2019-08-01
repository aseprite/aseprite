// Aseprite UI Library
// Copyright (C) 2001-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/component.h"
#include "ui/property.h"

namespace ui {

Component::Component()
{
}

Component::~Component()
{
}

PropertyPtr Component::getProperty(const std::string& name) const
{
  auto it = m_properties.find(name);
  if (it != m_properties.end())
    return it->second;
  else
    return PropertyPtr();
}

void Component::setProperty(PropertyPtr property)
{
  m_properties[property->getName()] = property;
}

bool Component::hasProperty(const std::string& name) const
{
  return (m_properties.find(name) != m_properties.end());
}

void Component::removeProperty(const std::string& name)
{
  Properties::iterator it = m_properties.find(name);
  if (it != m_properties.end())
    m_properties.erase(it);
}

const Component::Properties& Component::getProperties() const
{
  return m_properties;
}

} // namespace ui
