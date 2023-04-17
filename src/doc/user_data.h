// Aseprite Document Library
// Copyright (c) 2022-2023 Igara Studio S.A.
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_USER_DATA_H_INCLUDED
#define DOC_USER_DATA_H_INCLUDED
#pragma once

#include "base/uuid.h"
#include "doc/color.h"
#include "fixmath/fixmath.h"
#include "gfx/point.h"
#include "gfx/size.h"
#include "gfx/rect.h"

#include <cstddef>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#define USER_DATA_PROPERTY_TYPE_NULLPTR     0x0000
#define USER_DATA_PROPERTY_TYPE_BOOL        0x0001
#define USER_DATA_PROPERTY_TYPE_INT8        0x0002
#define USER_DATA_PROPERTY_TYPE_UINT8       0x0003
#define USER_DATA_PROPERTY_TYPE_INT16       0x0004
#define USER_DATA_PROPERTY_TYPE_UINT16      0x0005
#define USER_DATA_PROPERTY_TYPE_INT32       0x0006
#define USER_DATA_PROPERTY_TYPE_UINT32      0x0007
#define USER_DATA_PROPERTY_TYPE_INT64       0x0008
#define USER_DATA_PROPERTY_TYPE_UINT64      0x0009
#define USER_DATA_PROPERTY_TYPE_FIXED       0x000A
#define USER_DATA_PROPERTY_TYPE_FLOAT       0x000B
#define USER_DATA_PROPERTY_TYPE_DOUBLE      0x000C
#define USER_DATA_PROPERTY_TYPE_STRING      0x000D
#define USER_DATA_PROPERTY_TYPE_POINT       0x000E
#define USER_DATA_PROPERTY_TYPE_SIZE        0x000F
#define USER_DATA_PROPERTY_TYPE_RECT        0x0010
#define USER_DATA_PROPERTY_TYPE_VECTOR      0x0011
#define USER_DATA_PROPERTY_TYPE_PROPERTIES  0x0012
#define USER_DATA_PROPERTY_TYPE_UUID        0x0013

namespace doc {

  class UserData {
  public:
    struct Fixed {
      fixmath::fixed value;

      bool operator==(const Fixed& f) const {
        return this->value == f.value;
      }
    };
    struct Variant;
    using Vector = std::vector<Variant>;
    using Properties = std::map<std::string, Variant>;
    using PropertiesMaps = std::map<std::string, Properties>;
    using VariantBase = std::variant<std::nullptr_t,
                                     bool,
                                     int8_t, uint8_t,
                                     int16_t, uint16_t,
                                     int32_t, uint32_t,
                                     int64_t, uint64_t,
                                     Fixed,
                                     float, double,
                                     std::string,
                                     gfx::Point,
                                     gfx::Size,
                                     gfx::Rect,
                                     Vector,
                                     Properties,
                                     base::Uuid>;

    struct Variant : public VariantBase {
      Variant() = default;
      Variant(const Variant& v) = default;

      template<typename T>
      Variant(T&& v) : VariantBase(std::forward<T>(v)) { }

      // Avoid using Variant.operator=(const char*) because the "const
      // char*" is converted to a bool implicitly by MSVC.
      Variant& operator=(const char*) = delete;

      template<typename T>
      Variant& operator=(T&& v) {
        VariantBase::operator=(std::forward<T>(v));
        return *this;
      }

      const uint16_t type() const {
        return index();
      }
    };

    UserData() : m_color(0) {
    }

    size_t size() const { return m_text.size(); }
    bool isEmpty() const {
      return m_text.empty() && !doc::rgba_geta(m_color) && m_propertiesMaps.empty();
    }

    const std::string& text() const { return m_text; }
    color_t color() const { return m_color; }
    const PropertiesMaps& propertiesMaps() const { return m_propertiesMaps; }
    PropertiesMaps& propertiesMaps() { return m_propertiesMaps; }
    Properties& properties() { return properties(std::string()); }
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

  // macOS 10.9 C++ runtime doesn't support std::get<T>(value)
  // directly and we have to use std::get_if<T>(value)
  //
  // TODO replace this with std::get() in the future when we drop macOS 10.9 support
  template<typename T>
  inline const T& get_value(const UserData::Variant& variant) {
    const T* value = std::get_if<T>(&variant);
    if (value == nullptr)
      throw std::runtime_error("bad_variant_access");
    return *value;
  }

  template<typename T>
  inline T& get_value(UserData::Variant& variant) {
    T* value = std::get_if<T>(&variant);
    if (value == nullptr)
      throw std::runtime_error("bad_variant_access");
    return *value;
  }

  template<typename T, typename U>
  inline bool is_compatible_int(const U u) {
    static_assert(sizeof(U) > sizeof(T), "T type must be smaller than U type");
    return (u >= std::numeric_limits<T>::min() &&
            u <= std::numeric_limits<T>::max());
  }

  inline bool is_reducible_int(const UserData::Variant& variant) {
    return (variant.type() >= USER_DATA_PROPERTY_TYPE_INT16 &&
            variant.type() <= USER_DATA_PROPERTY_TYPE_UINT64);
  }

  size_t count_nonempty_properties_maps(const UserData::PropertiesMaps& propertiesMaps);

  // If all the elements of vector have the same type, returns that type, also
  // if this type is an integer, it tries to reduce it to the minimum int type
  // capable of storing all the vector values.
  // If all the elements of vector doesn't have the same type, returns 0.
  uint16_t all_elements_of_same_type(const UserData::Vector& vector);
  UserData::Variant cast_to_smaller_int_type(const UserData::Variant& value, uint16_t type);
  UserData::Variant reduce_int_type_size(const UserData::Variant& value);

} // namespace doc

#endif
