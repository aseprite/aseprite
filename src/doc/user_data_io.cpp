// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/user_data_io.h"

#include "base/serialization.h"
#include "doc/string_io.h"
#include "doc/user_data.h"

#include <iostream>

namespace doc {

using namespace base::serialization;
using namespace base::serialization::little_endian;

void write_properties_maps(std::ostream& os, const UserData::PropertiesMaps& propertiesMaps);
UserData::PropertiesMaps read_properties_maps(std::istream& is);

void write_user_data(std::ostream& os, const UserData& userData)
{
  write_string(os, userData.text());
  write32(os, userData.color());
  write_properties_maps(os, userData.propertiesMaps());
}

UserData read_user_data(std::istream& is)
{
  UserData userData;
  userData.setText(read_string(is));
  // This check is here because we've been restoring sprites from
  // old sessions where the color is restored incorrectly if we
  // don't check if there is enough space to read from the file
  // (e.g. reading a random color or just white, maybe -1 which is
  // 0xffffffff in 32-bit).
  if (!is.eof()) {
    userData.setColor(read32(is));
    userData.propertiesMaps() = read_properties_maps(is);
  }
  return userData;
}

size_t count_nonempty_properties_maps(const UserData::PropertiesMaps& propertiesMaps)
{
  size_t i = 0;
  for (const auto& it : propertiesMaps)
    if (!it.second.empty())
      ++i;
  return i;
}

bool is_negative(const UserData::Variant& value)
{
  switch (value.type()) {
    case USER_DATA_PROPERTY_TYPE_INT8: {
      auto v = get_value<int8_t>(value);
      return v < 0;
    }
    case USER_DATA_PROPERTY_TYPE_INT16: {
      auto v = get_value<int16_t>(value);
      return v < 0;
    }
    case USER_DATA_PROPERTY_TYPE_INT32: {
      auto v = get_value<int32_t>(value);
      return v < 0;
    }
    case USER_DATA_PROPERTY_TYPE_INT64: {
      auto v = get_value<int64_t>(value);
      return v < 0;
    }
  }
  return false;
}


// If all the elements of vector have the same type, returns that type, also
// if this type is an integer, it tries to reduce it to the minimum int type
// capable of storing all the vector values.
// If all the elements of vector doesn't have the same type, returns 0.
uint16_t all_elements_of_same_type(const UserData::Vector& vector)
{
  uint16_t type = vector.empty() ? 0 : vector.front().type();
  uint16_t commonReducedType = 0;
  bool hasNegativeNumbers = false;
  for (auto value : vector) {
    if (type != value.type()) {
      return 0;
    }
    else if (IS_REDUCIBLE_INT(value.type())) {
      auto t = reduce_int_type_size(value).type();
      hasNegativeNumbers |= is_negative(value);
      if (t > commonReducedType) {
        commonReducedType = t;
      }
    }
  }

  // TODO: The following check probably is not useful right now, I believe this could
  // become useful if at some point we want to try to find a common integer type for vectors
  // that contains elements of different integer types only.

  // If our common reduced type is unsigned and we have negative numbers
  // in our vector we should select the next signed type that includes it.
  if (commonReducedType != 0 &&
      (commonReducedType & 1) &&
      hasNegativeNumbers) {
    commonReducedType++;
    // We couldn't find one type that satisfies all the integers. This shouldn't ever happen.
    if (commonReducedType >= USER_DATA_PROPERTY_TYPE_UINT64) commonReducedType = 0;
  }

  return commonReducedType ? commonReducedType : type;
}

UserData::Variant cast_to_smaller_int_type(const UserData::Variant& value, uint16_t type)
{
  ASSERT(type < value.type());
  switch (value.type()) {
    case USER_DATA_PROPERTY_TYPE_INT16: {
      auto v = get_value<int16_t>(value);
      if (type == USER_DATA_PROPERTY_TYPE_INT8)
        return static_cast<int8_t>(v);
      else if (type == USER_DATA_PROPERTY_TYPE_UINT8)
        return static_cast<uint8_t>(v);
      break;
    }
    case USER_DATA_PROPERTY_TYPE_UINT16: {
      auto v = get_value<uint16_t>(value);
      if (type == USER_DATA_PROPERTY_TYPE_INT8)
        return static_cast<int8_t>(v);
      else if (type == USER_DATA_PROPERTY_TYPE_UINT8)
        return static_cast<uint8_t>(v);
      break;
    }
    case USER_DATA_PROPERTY_TYPE_INT32: {
      auto v = get_value<int32_t>(value);
      if (type == USER_DATA_PROPERTY_TYPE_INT8)
        return static_cast<int8_t>(v);
      else if (type == USER_DATA_PROPERTY_TYPE_UINT8)
        return static_cast<uint8_t>(v);
      else if (type == USER_DATA_PROPERTY_TYPE_INT16)
        return static_cast<int16_t>(v);
      else if (type == USER_DATA_PROPERTY_TYPE_UINT16)
        return static_cast<uint16_t>(v);
      break;
    }
    case USER_DATA_PROPERTY_TYPE_UINT32: {
      auto v = get_value<uint32_t>(value);
      if (type == USER_DATA_PROPERTY_TYPE_INT8)
        return static_cast<int8_t>(v);
      else if (type == USER_DATA_PROPERTY_TYPE_UINT8)
        return static_cast<uint8_t>(v);
      else if (type == USER_DATA_PROPERTY_TYPE_INT16)
        return static_cast<int16_t>(v);
      else if (type == USER_DATA_PROPERTY_TYPE_UINT16)
        return static_cast<uint16_t>(v);
      break;
    }
    case USER_DATA_PROPERTY_TYPE_INT64: {
      auto v = get_value<int64_t>(value);
      if (type == USER_DATA_PROPERTY_TYPE_INT8)
        return static_cast<int8_t>(v);
      else if (type == USER_DATA_PROPERTY_TYPE_UINT8)
        return static_cast<uint8_t>(v);
      else if (type == USER_DATA_PROPERTY_TYPE_INT16)
        return static_cast<int16_t>(v);
      else if (type == USER_DATA_PROPERTY_TYPE_UINT16)
        return static_cast<uint16_t>(v);
      else if (type == USER_DATA_PROPERTY_TYPE_INT32)
        return static_cast<int32_t>(v);
      else if (type == USER_DATA_PROPERTY_TYPE_UINT32)
        return static_cast<uint32_t>(v);
      break;
    }
    case USER_DATA_PROPERTY_TYPE_UINT64: {
      auto v = get_value<uint64_t>(value);
      if (type == USER_DATA_PROPERTY_TYPE_INT8)
        return static_cast<int8_t>(v);
      else if (type == USER_DATA_PROPERTY_TYPE_UINT8)
        return static_cast<uint8_t>(v);
      else if (type == USER_DATA_PROPERTY_TYPE_INT16)
        return static_cast<int16_t>(v);
      else if (type == USER_DATA_PROPERTY_TYPE_UINT16)
        return static_cast<uint16_t>(v);
      else if (type == USER_DATA_PROPERTY_TYPE_INT32)
        return static_cast<int32_t>(v);
      else if (type == USER_DATA_PROPERTY_TYPE_UINT32)
        return static_cast<uint32_t>(v);
      break;
    }
  }
  return value;
}

UserData::Variant reduce_int_type_size(const UserData::Variant& value)
{
  switch (value.type()) {
    case USER_DATA_PROPERTY_TYPE_INT16: {
      auto v = get_value<int16_t>(value);
      if (INT8_COMPATIBLE(v)) return static_cast<int8_t>(v);
      else if (UINT8_COMPATIBLE(v)) return static_cast<uint8_t>(v);
      return v;
    }
    case USER_DATA_PROPERTY_TYPE_UINT16: {
      auto v = get_value<uint16_t>(value);
      if (INT8_COMPATIBLE(v)) return static_cast<int8_t>(v);
      else if (UINT8_COMPATIBLE(v)) return static_cast<uint8_t>(v);
      return v;
    }
    case USER_DATA_PROPERTY_TYPE_INT32: {
      auto v = get_value<int32_t>(value);
      if (INT8_COMPATIBLE(v)) return static_cast<int8_t>(v);
      else if (UINT8_COMPATIBLE(v)) return static_cast<uint8_t>(v);
      else if (INT16_COMPATIBLE(v)) return static_cast<int16_t>(v);
      else if (UINT16_COMPATIBLE(v)) return static_cast<uint16_t>(v);
      return v;
    }
    case USER_DATA_PROPERTY_TYPE_UINT32: {
      auto v = get_value<uint32_t>(value);
      if (INT8_COMPATIBLE(v)) return static_cast<int8_t>(v);
      else if (UINT8_COMPATIBLE(v)) return static_cast<uint8_t>(v);
      else if (INT16_COMPATIBLE(v)) return static_cast<int16_t>(v);
      else if (UINT16_COMPATIBLE(v)) return static_cast<uint16_t>(v);
      return v;
    }
    case USER_DATA_PROPERTY_TYPE_INT64: {
      auto v = get_value<int64_t>(value);
      if (INT8_COMPATIBLE(v)) return static_cast<int8_t>(v);
      else if (UINT8_COMPATIBLE(v)) return static_cast<uint8_t>(v);
      else if (INT16_COMPATIBLE(v)) return static_cast<int16_t>(v);
      else if (UINT16_COMPATIBLE(v)) return static_cast<uint16_t>(v);
      else if (INT32_COMPATIBLE(v)) return static_cast<int32_t>(v);
      else if (UINT32_COMPATIBLE(v)) return static_cast<uint32_t>(v);
      return v;
    }
    case USER_DATA_PROPERTY_TYPE_UINT64: {
      auto v = get_value<uint64_t>(value);
      if (INT8_COMPATIBLE(v)) return static_cast<int8_t>(v);
      else if (UINT8_COMPATIBLE(v)) return static_cast<uint8_t>(v);
      else if (INT16_COMPATIBLE(v)) return static_cast<int16_t>(v);
      else if (UINT16_COMPATIBLE(v)) return static_cast<uint16_t>(v);
      else if (INT32_COMPATIBLE(v)) return static_cast<int32_t>(v);
      else if (UINT32_COMPATIBLE(v)) return static_cast<uint32_t>(v);
      return v;
    }
    default:
      return value;
  }
}

void write_point(std::ostream& os, const gfx::Point& point)
{
  write32(os, point.x);
  write32(os, point.y);
}

void write_size(std::ostream& os, const gfx::Size& size)
{
  write32(os, size.w);
  write32(os, size.h);
}

void write_property_value(std::ostream& os, const UserData::Variant& variant)
{
  switch (variant.type())
  {
    case USER_DATA_PROPERTY_TYPE_BOOL:
      write8(os, get_value<bool>(variant));
      break;
    case USER_DATA_PROPERTY_TYPE_INT8:
      write8(os, get_value<int8_t>(variant));
      break;
    case USER_DATA_PROPERTY_TYPE_UINT8:
      write8(os, get_value<uint8_t>(variant));
      break;
    case USER_DATA_PROPERTY_TYPE_INT16:
      write16(os, get_value<int16_t>(variant));
      break;
    case USER_DATA_PROPERTY_TYPE_UINT16:
      write16(os, get_value<uint16_t>(variant));
      break;
    case USER_DATA_PROPERTY_TYPE_INT32:
      write32(os, get_value<int32_t>(variant));
      break;
    case USER_DATA_PROPERTY_TYPE_UINT32:
      write32(os, get_value<uint32_t>(variant));
      break;
    case USER_DATA_PROPERTY_TYPE_INT64:
      write64(os, get_value<int64_t>(variant));
      break;
    case USER_DATA_PROPERTY_TYPE_UINT64:
      write64(os, get_value<uint64_t>(variant));
      break;
    case USER_DATA_PROPERTY_TYPE_FIXED:
      write32(os, get_value<UserData::Fixed>(variant).value);
      break;
    case USER_DATA_PROPERTY_TYPE_FLOAT:
      write_float(os, get_value<float>(variant));
      break;
    case USER_DATA_PROPERTY_TYPE_DOUBLE:
      write_double(os, get_value<double>(variant));
      break;
    case USER_DATA_PROPERTY_TYPE_STRING:
      write_string(os, get_value<std::string>(variant));
      break;
    case USER_DATA_PROPERTY_TYPE_POINT:
      write_point(os, get_value<gfx::Point>(variant));
      break;
    case USER_DATA_PROPERTY_TYPE_SIZE:
      write_size(os, get_value<gfx::Size>(variant));
      break;
    case USER_DATA_PROPERTY_TYPE_RECT: {
      const gfx::Rect& rect = get_value<gfx::Rect>(variant);
      write_point(os, rect.origin());
      write_size(os, rect.size());
      break;
    }
    case USER_DATA_PROPERTY_TYPE_VECTOR: {
        const std::vector<UserData::Variant>& vector = get_value<std::vector<UserData::Variant>>(variant);
        write32(os, vector.size());
        for (auto elem : vector) {
          write16(os, elem.type());
          write_property_value(os, elem);
        }
      break;
    }
    case USER_DATA_PROPERTY_TYPE_PROPERTIES: {
      auto properties = get_value<UserData::Properties>(variant);
      write32(os, properties.size());
      for (auto property : properties) {
        const std::string& name = property.first;
        write_string(os, name);

        const UserData::Variant& value = property.second;
        write16(os, value.type());

        write_property_value(os, value);
      }
      break;
    }
  }
}

void write_properties_maps(std::ostream& os, const UserData::PropertiesMaps& propertiesMaps)
{
  write32(os, propertiesMaps.size());
  for (auto propertiesMap : propertiesMaps) {
    const UserData::Properties& properties = propertiesMap.second;
    const std::string& extensionKey = propertiesMap.first;
    write_string(os, extensionKey);
    write_property_value(os, properties);
  }
}

UserData::Variant read_property_value(std::istream& is, uint16_t type)
{
  switch (type) {
    case USER_DATA_PROPERTY_TYPE_NULLPTR: {
      return nullptr;
    }
    case USER_DATA_PROPERTY_TYPE_BOOL: {
      bool value = read8(is);
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_INT8: {
      int8_t value = read8(is);
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_UINT8: {
      uint8_t value = read8(is);
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_INT16: {
      int16_t value = read16(is);
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_UINT16: {
      uint16_t value = read16(is);
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_INT32: {
      int32_t value = read32(is);
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_UINT32: {
      uint32_t value = read32(is);
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_INT64: {
      int64_t value = read64(is);
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_UINT64: {
      uint64_t value = read64(is);
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_FIXED: {
      int32_t value = read32(is);
      return doc::UserData::Fixed{value};
    }
    case USER_DATA_PROPERTY_TYPE_FLOAT: {
      float value = read_float(is);
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_DOUBLE: {
      double value = read_double(is);
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_STRING: {
      std::string value = read_string(is);
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_POINT: {
      int32_t x = read32(is);
      int32_t y = read32(is);
      return gfx::Point(x, y);
    }
    case USER_DATA_PROPERTY_TYPE_SIZE: {
      int32_t w = read32(is);
      int32_t h = read32(is);
      return gfx::Size(w, h);
    }
    case USER_DATA_PROPERTY_TYPE_RECT: {
      int32_t x = read32(is);
      int32_t y = read32(is);
      int32_t w = read32(is);
      int32_t h = read32(is);
      return gfx::Rect(x, y, w, h);
    }
    case USER_DATA_PROPERTY_TYPE_VECTOR: {
      auto numElems = read32(is);
      std::vector<doc::UserData::Variant> value;
      for (int k=0; k<numElems;++k) {
        auto elemType = read16(is);
        value.push_back(read_property_value(is, elemType));
      }
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_PROPERTIES: {
      auto numProps = read32(is);
      doc::UserData::Properties value;
      for (int j=0; j<numProps;++j) {
        auto name = read_string(is);
        auto type = read16(is);
        value[name] = read_property_value(is, type);
      }
      return value;
    }
  }

  return doc::UserData::Variant{};
}

UserData::PropertiesMaps read_properties_maps(std::istream& is)
{
  doc::UserData::PropertiesMaps propertiesMaps;
  size_t nmaps = read32(is);
  for (int i=0; i<nmaps; ++i) {
    std::string extensionId = read_string(is);
    auto properties = read_property_value(is, USER_DATA_PROPERTY_TYPE_PROPERTIES);
    propertiesMaps[extensionId] = doc::get_value<doc::UserData::Properties>(properties);
  }
  return propertiesMaps;
}

}
