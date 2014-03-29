// Aseprite CSS Library
// Copyright (C) 2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef CSS_VALUE_H_INCLUDED
#define CSS_VALUE_H_INCLUDED
#pragma once

#include "css/map.h"

#include <string>

namespace css {

  class Value {
  public:
    enum Type {
      None,
      Number,
      String
    };

    Value();
    explicit Value(double value, const std::string& unit = "");
    explicit Value(const std::string& value);

    Type type() const { return m_type; }
    
    double number() const;
    std::string string() const;
    std::string unit() const;

    void setNumber(double value);
    void setString(const std::string& value);
    void setUnit(const std::string& unit = "");

    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const {
      return !operator==(other);
    }

  private:
    Type m_type;
    double m_number;
    std::string m_string;
  };

  typedef Map<Value> Values;

} // namespace css

#endif
