// Aseprite FreeType Wrapper
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef FT_HB_SHAPER_H_INCLUDED
#define FT_HB_SHAPER_H_INCLUDED
#pragma once

#include "ft/face.h"

#include <hb.h>
#include <hb-ft.h>

#include <string>

namespace ft {

  template<typename FaceFT>
  class HBShaper {
  public:
    HBShaper(FaceFT& face) {
      m_font = hb_ft_font_create((FT_Face)face, nullptr);
      m_buffer = hb_buffer_create();
    }

    ~HBShaper() {
      if (m_buffer) hb_buffer_destroy(m_buffer);
      if (m_font) hb_font_destroy(m_font);
    }

    bool initialize(const base::utf8_const_iterator& it,
                    const base::utf8_const_iterator& end) {
      m_it = it;
      m_end = end;
      m_index = 0;

      hb_buffer_reset(m_buffer);
      for (auto it=m_it; it!=end; ++it)
        hb_buffer_add(m_buffer, *it, 0);
      hb_buffer_set_content_type(m_buffer, HB_BUFFER_CONTENT_TYPE_UNICODE);
      hb_buffer_set_direction(m_buffer, HB_DIRECTION_LTR);
      hb_buffer_guess_segment_properties(m_buffer);

      hb_shape(m_font, m_buffer, nullptr, 0);

      m_glyphInfo = hb_buffer_get_glyph_infos(m_buffer, &m_glyphCount);
      m_glyphPos = hb_buffer_get_glyph_positions(m_buffer, &m_glyphCount);
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
    hb_buffer_t* m_buffer;
    hb_font_t* m_font;
    hb_glyph_info_t* m_glyphInfo;
    hb_glyph_position_t* m_glyphPos;
    unsigned int m_glyphCount;
    int m_index;
    base::utf8_const_iterator m_it;
    base::utf8_const_iterator m_end;
  };

} // namespace ft

#endif
