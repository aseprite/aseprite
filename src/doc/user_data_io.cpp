// Aseprite Document Library
// Copyright (c) 2023 Igara Studio S.A.
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

static void write_point(std::ostream& os, const gfx::Point& point)
{
  write32(os, point.x);
  write32(os, point.y);
}

static void write_size(std::ostream& os, const gfx::Size& size)
{
  write32(os, size.w);
  write32(os, size.h);
}

static void write_property_value(std::ostream& os, const UserData::Variant& variant)
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
    case USER_DATA_PROPERTY_TYPE_UUID: {
      auto uuid = get_value<base::Uuid>(variant);
      for (int i=0; i<16; ++i) {
        write8(os, uuid[i]);
      }
      break;
    }
  }
}

static void write_properties_maps(std::ostream& os, const UserData::PropertiesMaps& propertiesMaps)
{
  write32(os, propertiesMaps.size());
  for (auto propertiesMap : propertiesMaps) {
    const UserData::Properties& properties = propertiesMap.second;
    const std::string& extensionKey = propertiesMap.first;
    write_string(os, extensionKey);
    write_property_value(os, properties);
  }
}

void write_user_data(std::ostream& os, const UserData& userData)
{
  write_string(os, userData.text());
  write32(os, userData.color());
  write_properties_maps(os, userData.propertiesMaps());
}

static UserData::Variant read_property_value(std::istream& is, uint16_t type)
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
    case USER_DATA_PROPERTY_TYPE_UUID: {
      base::Uuid value;
      uint8_t* bytes = value.bytes();
      for (int i=0; i<16; ++i) {
        bytes[i] = read8(is);
      }
      return value;
    }
  }

  return doc::UserData::Variant{};
}

static UserData::PropertiesMaps read_properties_maps(std::istream& is)
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

UserData read_user_data(std::istream& is, const int docFormatVer)
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
    // When recovering a session from an old Aseprite version, we need
    // to skip reading the parts that it doesn't contains. Otherwise
    // it is very likely to fail.
    if (docFormatVer >= DOC_FORMAT_VERSION_2) {
      userData.propertiesMaps() = read_properties_maps(is);
    }
  }
  return userData;
}

}
