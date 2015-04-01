// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_SKIN_SKIN_PART_H_INCLUDED
#define APP_UI_SKIN_SKIN_PART_H_INCLUDED
#pragma once

#include "base/shared_ptr.h"

#include <vector>

namespace she {
  class Surface;
}

namespace app {
  namespace skin {

    class SkinPart {
    public:
      typedef std::vector<she::Surface*> Bitmaps;

      SkinPart();
      ~SkinPart();

      std::size_t size() const { return m_bitmaps.size(); }

      void clear();

      // It doesn't destroy the previous bitmap in the given "index".
      void setBitmap(std::size_t index, she::Surface* bitmap);

      she::Surface* getBitmap(std::size_t index) const {
        return (index < m_bitmaps.size() ? m_bitmaps[index]: NULL);
      }

    private:
      Bitmaps m_bitmaps;
    };

    typedef base::SharedPtr<SkinPart> SkinPartPtr;

  } // namespace skin
} // namespace app

#endif
