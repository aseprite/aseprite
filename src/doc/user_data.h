// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_USER_DATA_H_INCLUDED
#define DOC_USER_DATA_H_INCLUDED
#pragma once

#include "doc/color.h"
#include "fixmath/fixmath.h"
#include "gfx/point.h"
#include "gfx/size.h"
#include "gfx/rect.h"

#include <map>
#include <string>
#include <variant>
#include <vector>

namespace doc {
  struct VariantStruct;
  struct Fixed {
    fixmath::fixed value;
  };
  using Variant = VariantStruct;
  using Property = Variant;
  struct Properties : std::map<std::string, Variant> {

    template<typename T>
    T& property(const std::string& propName) {
      return *(std::get_if<T>(&this->at(propName)));
    }

//    const uint16_t type(const std::string& propName) const {
//      return this->at(propName)->index();
//      return 1;
//    }
  };
  struct VariantStruct : std::variant<bool,
                                      int8_t, uint8_t,
                                      int16_t, uint16_t,
                                      int32_t, uint32_t,
                                      int64_t, uint64_t,
                                      Fixed,
                                      std::string,
                                      gfx::Point,
                                      gfx::Size,
                                      gfx::Rect,
                                      std::vector<Variant>,
                                      Properties>{
    const uint16_t type() const {
      return index() + 1;
    }
  };
  using PropertiesMaps = std::map<std::string, Properties>;

  class UserData {
  public:

    UserData()
      : m_text("")
      , m_color(0) {
      setProperty("Extension111", "Property111", Variant{uint32_t(1111)});// test values, only for development
      setProperty("Extension111", "Property222", Variant{uint32_t(2222)});
      setProperty("Extension222", "Property333", Variant{uint32_t(3333)});
    }

    UserData(const UserData& userData)
      : m_text(userData.text())
      , m_color(userData.color())
      , m_propertiesMaps(userData.propertiesMaps()) {
    }

    size_t size() const { return m_text.size(); }
    bool isEmpty() const {
      return m_text.empty() && !doc::rgba_geta(m_color) && m_propertiesMaps.empty();
    }

    const std::string& text() const { return m_text; }
    color_t color() const { return m_color; }
    const PropertiesMaps& propertiesMaps() const { return m_propertiesMaps; }
    Properties& properties() { return properties(""); }
    Properties& properties(const std::string& extensionName) { return m_propertiesMaps.at(extensionName); }

    template<typename T>
    T& getProperty(const std::string& extensionName, const std::string& propName) {
      return *(std::get_if<T>(&m_propertiesMaps[extensionName][propName]));
    }

    void setText(const std::string& text) { m_text = text; }
    void setColor(color_t color) { m_color = color; }
    void setProperty(const std::string& extensionName,
                     const std::string& propName,
                     const Variant& propValue) {
      m_propertiesMaps[extensionName][propName] = propValue;
    }

    bool operator==(const UserData& other) const {
      return (m_text == other.m_text &&
              m_color == other.m_color);
    }

    bool operator!=(const UserData& other) const {
      return !operator==(other);
    }

  private:
    std::string m_text;
    color_t m_color;
    PropertiesMaps m_propertiesMaps;
  };

} // namespace doc

#endif
