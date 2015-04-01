// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_PROPERTY_H_INCLUDED
#define UI_PROPERTY_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "base/shared_ptr.h"

#include <string>

namespace ui {

  class Property {
  public:
    Property(const std::string& name);
    virtual ~Property();

    std::string getName() const;

  private:
    std::string m_name;

    DISABLE_COPYING(Property);
  };

  typedef base::SharedPtr<Property> PropertyPtr;

} // namespace ui

#endif
