// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/i18n/strings.h"
#include "app/tools/ink_type.h"

namespace app { namespace tools {

std::string ink_type_to_string(InkType inkType)
{
  switch (inkType) {
    case tools::InkType::SIMPLE:            return Strings::inks_simple_ink();
    case tools::InkType::ALPHA_COMPOSITING: return Strings::inks_alpha_compositing();
    case tools::InkType::COPY_COLOR:        return Strings::inks_copy_color();
    case tools::InkType::LOCK_ALPHA:        return Strings::inks_lock_alpha();
    case tools::InkType::SHADING:           return Strings::inks_shading();
  }
  return Strings::general_unknown();
}

std::string ink_type_to_string_id(InkType inkType)
{
  switch (inkType) {
    case tools::InkType::SIMPLE:            return "simple";
    case tools::InkType::ALPHA_COMPOSITING: return "alpha_compositing";
    case tools::InkType::COPY_COLOR:        return "copy_color";
    case tools::InkType::LOCK_ALPHA:        return "lock_alpha";
    case tools::InkType::SHADING:           return "shading";
  }
  return "unknown";
}

InkType string_id_to_ink_type(const std::string& s)
{
  if (s == "simple")
    return tools::InkType::SIMPLE;

  if (s == "alpha_compositing" || s == "alpha-compositing")
    return tools::InkType::ALPHA_COMPOSITING;

  if (s == "copy_color" || s == "copy-color")
    return tools::InkType::COPY_COLOR;

  if (s == "lock_alpha" || s == "lock-alpha")
    return tools::InkType::LOCK_ALPHA;

  if (s == "shading")
    return tools::InkType::SHADING;

  return tools::InkType::DEFAULT;
}

}} // namespace app::tools
