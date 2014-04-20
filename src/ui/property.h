// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_PROPERTY_H_INCLUDED
#define UI_PROPERTY_H_INCLUDED
#pragma once

#include "base/string.h"
#include "base/disable_copying.h"
#include "base/shared_ptr.h"

namespace ui {

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

} // namespace ui

#endif
