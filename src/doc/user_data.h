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

  class UserData {
  public:
    struct Fixed {
      fixmath::fixed value;
    };
    struct VariantStruct;
    using Variant = VariantStruct;
    using Properties = std::map<std::string, Variant>;
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
                                        Properties>{};
    using PropertiesMaps = std::map<std::string, Properties>;

    UserData() : m_color(0) {
    }

    size_t size() const { return m_text.size(); }
    bool isEmpty() const {
      return m_text.empty() && !doc::rgba_geta(m_color) && m_propertiesMaps.empty();
    }

    const std::string& text() const { return m_text; }
    color_t color() const { return m_color; }
    const PropertiesMaps& propertiesMaps() const { return m_propertiesMaps; }
    Properties& properties() { return properties(""); }
    Properties& properties(const std::string& groupKey) { return m_propertiesMaps[groupKey]; }

    void setText(const std::string& text) { m_text = text; }
    void setColor(color_t color) { m_color = color; }

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
