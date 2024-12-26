// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_HIT_H_INCLUDED
#define APP_UI_EDITOR_HIT_H_INCLUDED
#pragma once

namespace doc {
class Slice;
}

namespace app {

// TODO Complete this class to calculate if the mouse hit any Editor
//      element/decorator
class EditorHit {
public:
  enum Type {
    None,
    SliceBounds,
    SliceCenter,
  };

  EditorHit(Type type) : m_type(type), m_border(0), m_slice(nullptr) {}

  Type type() const { return m_type; }
  int border() const { return m_border; }
  doc::Slice* slice() const { return m_slice; }

  void setBorder(int border) { m_border = border; }
  void setSlice(doc::Slice* slice) { m_slice = slice; }

private:
  Type m_type;
  int m_border;
  doc::Slice* m_slice;
};

} // namespace app

#endif // APP_UI_EDITOR_HIT_H_INCLUDED
