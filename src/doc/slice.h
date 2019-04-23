// Aseprite Document Library
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_SLICE_H_INCLUDED
#define DOC_SLICE_H_INCLUDED
#pragma once

#include "doc/frame.h"
#include "doc/keyframes.h"
#include "doc/with_user_data.h"
#include "gfx/point.h"
#include "gfx/rect.h"

#include <string>

namespace doc {
  class Slices;

  class SliceKey {
  public:
    static const gfx::Point NoPivot;

    SliceKey();
    SliceKey(const gfx::Rect& bounds,
             const gfx::Rect& center = gfx::Rect(),
             const gfx::Point& pivot = SliceKey::NoPivot);

    bool isEmpty() const { return m_bounds.isEmpty(); }
    bool hasCenter() const { return !m_center.isEmpty(); }
    bool hasPivot() const { return m_pivot != NoPivot; }

    const gfx::Rect& bounds() const { return m_bounds; }
    const gfx::Rect& center() const { return m_center; }
    const gfx::Point& pivot() const { return m_pivot; }

    void setBounds(const gfx::Rect& bounds) { m_bounds = bounds; }
    void setCenter(const gfx::Rect& center) { m_center = center; }
    void setPivot(const gfx::Point& pivot) { m_pivot = pivot; }

  private:
    gfx::Rect m_bounds;
    gfx::Rect m_center;
    gfx::Point m_pivot;
  };

  class Slice : public WithUserData {
  public:
    typedef Keyframes<SliceKey> List;
    typedef List::iterator iterator;
    typedef List::const_iterator const_iterator;

    Slice();
    ~Slice();

    int getMemSize() const override;

    void insert(const frame_t frame, const SliceKey& key);
    void remove(const frame_t frame);

    const SliceKey* getByFrame(const frame_t frame) const;
    iterator getIteratorByFrame(const frame_t frame) const;

    iterator begin() { return m_keys.begin(); }
    iterator end() { return m_keys.end(); }
    const_iterator begin() const { return m_keys.begin(); }
    const_iterator end() const { return m_keys.end(); }

    std::size_t size() const { return m_keys.size(); }
    bool empty() const { return m_keys.empty(); }

    Slices* owner() const { return m_owner; }
    frame_t fromFrame() const { return m_keys.fromFrame(); }
    frame_t toFrame() const { return m_keys.toFrame(); }
    const std::string& name() const { return m_name; }

    void setOwner(Slices* owner);
    void setName(const std::string& name);

    List::Range range(const frame_t from, const frame_t to) {
      return m_keys.range(from, to);
    }

  private:
    Slices* m_owner;
    std::string m_name;
    List m_keys;

    DISABLE_COPYING(Slice);
  };

} // namespace doc

#endif
