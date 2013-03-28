// ASEPRITE gui library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "ui/property.h"

namespace ui {

Property::Property(const base::string& name)
  : m_name(name)
{
}

Property::~Property()
{
}

base::string Property::getName() const
{
  return m_name;
}

} // namespace ui
