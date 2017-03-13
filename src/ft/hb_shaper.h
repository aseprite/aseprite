// Aseprite FreeType Wrapper
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef FT_HB_SHAPER_H_INCLUDED
#define FT_HB_SHAPER_H_INCLUDED
#pragma once

#include "ft/hb_face.h"

#include <vector>

namespace ft {

  template<typename HBFace>
  class HBShaper {
  public:
    HBShaper(HBFace& face)
      : m_face(face)
      , m_unicodeFuncs(hb_buffer_get_unicode_funcs(face.buffer())) {
    }

    bool initialize(const base::utf8_const_iterator& begin,
                    const base::utf8_const_iterator& end) {
      m_index = 0;
      if (begin == end)
        return false;

      hb_buffer_t* buf = m_face.buffer();

      hb_buffer_reset(buf);
      for (auto it=begin; it!=end; ++it)
        hb_buffer_add(buf, *it, it - begin);
      hb_buffer_set_cluster_level(buf, HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS);
      hb_buffer_set_content_type(buf, HB_BUFFER_CONTENT_TYPE_UNICODE);
      hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
      hb_buffer_guess_segment_properties(buf);

      hb_shape(m_face.font(), buf, nullptr, 0);

      m_glyphInfo = hb_buffer_get_glyph_infos(buf, &m_glyphCount);
      m_glyphPos = hb_buffer_get_glyph_positions(buf, &m_glyphCount);
      return (m_glyphCount > 0);
    }

    bool nextChar() {
      ++m_index;
      return (m_index < m_glyphCount);
    }

    int unicodeChar() const {
      return m_glyphInfo[m_index].codepoint;
    }

    int charIndex() {
      return m_glyphInfo[m_index].cluster;
    }

    unsigned int glyphIndex() const {
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
    unsigned int m_index;
    hb_unicode_funcs_t* m_unicodeFuncs;
  };

} // namespace ft

#endif
