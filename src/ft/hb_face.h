// Aseprite FreeType Wrapper
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef FT_HB_FACE_H_INCLUDED
#define FT_HB_FACE_H_INCLUDED
#pragma once

#include "base/string.h"
#include "ft/face.h"

#include <hb.h>
#include <hb-ft.h>

namespace ft {

  template<typename FaceFT>
  class HBFace : public FaceFT {
  public:
    HBFace(FT_Face face) : FaceFT(face) {
      m_font = hb_ft_font_create((FT_Face)face, nullptr);
      m_buffer = hb_buffer_create();
    }

    ~HBFace() {
      if (m_buffer) hb_buffer_destroy(m_buffer);
      if (m_font) hb_font_destroy(m_font);
    }

    hb_font_t* font() const { return m_font; }
    hb_buffer_t* buffer() const { return m_buffer; }

  private:
    hb_buffer_t* m_buffer;
    hb_font_t* m_font;
  };

  typedef HBFace<FaceFT<SimpleCache> > Face;

} // namespace ft

#endif
