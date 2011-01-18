// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "gui/property.h"

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
