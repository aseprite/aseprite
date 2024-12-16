// Aseprite Document Library
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_SLICES_H_INCLUDED
#define DOC_SLICES_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "doc/object_id.h"

#include <string>
#include <vector>

namespace doc {

class Slice;
class Sprite;

class Slices {
  typedef std::vector<Slice*> List;

public:
  typedef List::iterator iterator;
  typedef List::const_iterator const_iterator;

  Slices(Sprite* sprite);
  ~Slices();

  Sprite* sprite() { return m_sprite; }

  void add(Slice* slice);
  void remove(Slice* slice);

  Slice* getByName(const std::string& name) const;
  Slice* getById(const ObjectId id) const;

  iterator begin() { return m_slices.begin(); }
  iterator end() { return m_slices.end(); }
  const_iterator begin() const { return m_slices.begin(); }
  const_iterator end() const { return m_slices.end(); }

  std::size_t size() const { return m_slices.size(); }
  bool empty() const { return m_slices.empty(); }

private:
  Sprite* m_sprite;
  List m_slices;

  DISABLE_COPYING(Slices);
};

} // namespace doc

#endif
