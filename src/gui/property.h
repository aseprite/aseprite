// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_PROPERTY_H_INCLUDED
#define GUI_PROPERTY_H_INCLUDED

#include "base/string.h"
#include "base/disable_copying.h"
#include "base/shared_ptr.h"

class Property
{
  base::string m_name;

public:
  Property(const base::string& name);
  virtual ~Property();

  base::string getName() const;

private:
  DISABLE_COPYING(Property);
};

typedef SharedPtr<Property> PropertyPtr;

#endif
