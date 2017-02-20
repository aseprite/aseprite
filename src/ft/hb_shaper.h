// Aseprite FreeType Wrapper
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef FT_HB_SHAPER_H_INCLUDED
#define FT_HB_SHAPER_H_INCLUDED
#pragma once

#include "ft/hb_face.h"

namespace ft {

  template<typename HBFace>
  class HBShaper {
  public:
    HBShaper(HBFace& face) : m_face(face) {
    }

    bool initialize(const base::utf8_const_iterator& it,
                    const base::utf8_const_iterator& end) {
      m_it = it;
      m_end = end;
      m_index = 0;

      hb_buffer_reset(m_face.buffer());
      for (auto it=m_it; it!=end; ++it)
        hb_buffer_add(m_face.buffer(), *it, 0);
      hb_buffer_set_content_type(m_face.buffer(), HB_BUFFER_CONTENT_TYPE_UNICODE);
      hb_buffer_set_direction(m_face.buffer(), HB_DIRECTION_LTR);
      hb_buffer_guess_segment_properties(m_face.buffer());
      hb_shape(m_face.font(), m_face.buffer(), nullptr, 0);

      m_glyphInfo = hb_buffer_get_glyph_infos(m_face.buffer(), &m_glyphCount);
      m_glyphPos = hb_buffer_get_glyph_positions(m_face.buffer(), &m_glyphCount);
      return (m_glyphCount > 0);
    }

    bool nextChar() {
      ++m_it;
      ++m_index;
      return (m_index < m_glyphCount);
    }

    int unicodeChar() const {
      return *m_it;
    }

    unsigned int glyphIndex() {
      return m_glyphInfo[m_index].codepoint;
    }

    void glyphOffsetXY(Glyph* glyph) {
      glyph->x += m_glyphPos[m_index].x_offset / 64.0;
      glyph->y += m_glyphPos[m_index].y_offset / 64.0;
    }

    void glyphAdvanceXY(const Glyph* glyph, double& x, double& y) {
      x += m_glyphPos[m_index].x_advance / 64.0;
      y += m_glyphPos[m_index].y_advance / 64.0;
    }

  private:
    HBFace& m_face;
    hb_glyph_info_t* m_glyphInfo;
    hb_glyph_position_t* m_glyphPos;
    unsigned int m_glyphCount;
    int m_index;
    base::utf8_const_iterator m_it;
    base::utf8_const_iterator m_end;
  };

} // namespace ft

#endif
