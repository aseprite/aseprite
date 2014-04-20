// Aseprite CSS Library
// Copyright (C) 2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "css/value.h"

namespace css {

Value::Value() :
  m_type(None)
{
}

Value::Value(double value, const std::string& unit) :
  m_type(Number),
  m_number(value),
  m_string(unit)
{
}

Value::Value(const std::string& value) :
  m_type(String),
  m_string(value)
{
}
    
double Value::number() const
{
  if (m_type == Number)
    return m_number;
  else
    return 0.0;
}

std::string Value::string() const
{
  if (m_type == String)
    return m_string;
  else
    return std::string();
}

std::string Value::unit() const
{
  if (m_type == Number)
    return m_string;
  else
    return std::string();
}

void Value::setNumber(double value)
{
  if (m_type != Number) {
    m_type = Number;
    m_string = "";
  }
  m_number = value;
}

void Value::setString(const std::string& value)
{
  m_type = String;
  m_string = value;
}

void Value::setUnit(const std::string& unit)
{
  if (m_type != Number) {
    m_type = Number;
    m_number = 0.0;
  }
  m_string = unit;
}

bool Value::operator==(const Value& other) const
{
  if (m_type != other.m_type)
    return false;

  switch (m_type) {
    case None:
      return true;
    case Number:
      return m_number == other.m_number && m_string == other.m_string;
    case String:
      return m_string == other.m_string;
    default:
      return false;
  }
}
  
} // namespace css
