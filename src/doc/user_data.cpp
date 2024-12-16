// Aseprite Document Library
// Copyright (c) 2023  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/user_data.h"

#include "base/debug.h"

namespace doc {

size_t count_nonempty_properties_maps(const UserData::PropertiesMaps& propertiesMaps)
{
  size_t i = 0;
  for (const auto& it : propertiesMaps)
    if (!it.second.empty())
      ++i;
  return i;
}

static bool is_negative(const UserData::Variant& value)
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

uint16_t all_elements_of_same_type(const UserData::Vector& vector)
{
  uint16_t type = vector.empty() ? 0 : vector.front().type();
  uint16_t commonReducedType = 0;
  bool hasNegativeNumbers = false;

  for (const auto& value : vector) {
    if (type != value.type()) {
      return 0;
    }
    else if (is_reducible_int(value)) {
      auto t = reduce_int_type_size(value).type();
      hasNegativeNumbers |= is_negative(value);
      if (commonReducedType < t) {
        commonReducedType = t;
      }
    }
  }

  // TODO: The following check probably is not useful right now, I believe this could
  // become useful if at some point we want to try to find a common integer type for vectors
  // that contains elements of different integer types only.

  // If our common reduced type is unsigned and we have negative numbers
  // in our vector we should select the next signed type that includes it.
  if (commonReducedType != 0 && (commonReducedType & 1) && // TODO fix this assumption about
                                                           // USER_DATA_PROPERTY_TYPE_* values
      hasNegativeNumbers) {
    commonReducedType++;
    // We couldn't find one type that satisfies all the integers. This
    // shouldn't ever happen.
    if (commonReducedType >= USER_DATA_PROPERTY_TYPE_UINT64)
      commonReducedType = 0;
  }

  return (commonReducedType ? commonReducedType : type);
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
      if (is_compatible_int<int8_t>(v))
        return static_cast<int8_t>(v);
      else if (is_compatible_int<uint8_t>(v))
        return static_cast<uint8_t>(v);
      return v;
    }
    case USER_DATA_PROPERTY_TYPE_UINT16: {
      auto v = get_value<uint16_t>(value);
      if (is_compatible_int<int8_t>(v))
        return static_cast<int8_t>(v);
      else if (is_compatible_int<uint8_t>(v))
        return static_cast<uint8_t>(v);
      return v;
    }
    case USER_DATA_PROPERTY_TYPE_INT32: {
      auto v = get_value<int32_t>(value);
      if (is_compatible_int<int8_t>(v))
        return static_cast<int8_t>(v);
      else if (is_compatible_int<uint8_t>(v))
        return static_cast<uint8_t>(v);
      else if (is_compatible_int<int16_t>(v))
        return static_cast<int16_t>(v);
      else if (is_compatible_int<uint16_t>(v))
        return static_cast<uint16_t>(v);
      return v;
    }
    case USER_DATA_PROPERTY_TYPE_UINT32: {
      auto v = get_value<uint32_t>(value);
      if (is_compatible_int<int8_t>(v))
        return static_cast<int8_t>(v);
      else if (is_compatible_int<uint8_t>(v))
        return static_cast<uint8_t>(v);
      else if (is_compatible_int<int16_t>(v))
        return static_cast<int16_t>(v);
      else if (is_compatible_int<uint16_t>(v))
        return static_cast<uint16_t>(v);
      return v;
    }
    case USER_DATA_PROPERTY_TYPE_INT64: {
      auto v = get_value<int64_t>(value);
      if (is_compatible_int<int8_t>(v))
        return static_cast<int8_t>(v);
      else if (is_compatible_int<uint8_t>(v))
        return static_cast<uint8_t>(v);
      else if (is_compatible_int<int16_t>(v))
        return static_cast<int16_t>(v);
      else if (is_compatible_int<uint16_t>(v))
        return static_cast<uint16_t>(v);
      else if (is_compatible_int<int32_t>(v))
        return static_cast<int32_t>(v);
      else if (is_compatible_int<uint32_t>(v))
        return static_cast<uint32_t>(v);
      return v;
    }
    case USER_DATA_PROPERTY_TYPE_UINT64: {
      auto v = get_value<uint64_t>(value);
      if (is_compatible_int<int8_t>(v))
        return static_cast<int8_t>(v);
      else if (is_compatible_int<uint8_t>(v))
        return static_cast<uint8_t>(v);
      else if (is_compatible_int<int16_t>(v))
        return static_cast<int16_t>(v);
      else if (is_compatible_int<uint16_t>(v))
        return static_cast<uint16_t>(v);
      else if (is_compatible_int<int32_t>(v))
        return static_cast<int32_t>(v);
      else if (is_compatible_int<uint32_t>(v))
        return static_cast<uint32_t>(v);
      return v;
    }
    default: return value;
  }
}

} // namespace doc
