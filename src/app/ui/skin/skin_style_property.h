// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_SKIN_SKIN_STYLE_PROPERTY_H_INCLUDED
#define APP_UI_SKIN_SKIN_STYLE_PROPERTY_H_INCLUDED
#pragma once

#include "app/ui/skin/skin_property.h"
#include "app/ui/skin/style.h"
#include "base/shared_ptr.h"

namespace app {
namespace skin {
  class Style;

  class SkinStyleProperty : public ui::Property {
  public:
    static const char* Name;

    SkinStyleProperty(Style* style);

    skin::Style* getStyle() const;

  private:
    skin::Style* m_style;
  };

  typedef base::SharedPtr<SkinStyleProperty> SkinStylePropertyPtr;

} // namespace skin
} // namespace app

#endif
