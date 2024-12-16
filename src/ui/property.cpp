// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "ui/property.h"

namespace ui {

Property::Property(const std::string& name) : m_name(name)
{
}

Property::~Property()
{
}

std::string Property::getName() const
{
  return m_name;
}

} // namespace ui
