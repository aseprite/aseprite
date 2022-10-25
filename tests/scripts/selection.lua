-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

-- Isolated selection
do
  local a = Selection()
  assert(a.bounds.x == 0)
  assert(a.bounds.y == 0)
  assert(a.bounds.width == 0)
  assert(a.bounds.height == 0)
  assert(a.isEmpty)

  a:select(1, 2, 3, 4)
  assert(a.bounds.x == 1)
  assert(a.bounds.y == 2)
  assert(a.bounds.width == 3)
  assert(a.bounds.height == 4)
  assert(not a.isEmpty)
  assert(a:contains(1, 2))
  assert(a:contains(1+3-1, 2+4-1))
  assert(not a:contains(0, 1))
  assert(not a:contains(1+3, 2+4))

  a:select{x=5, y=6, width=7, height=8}
  assert(a.bounds.x == 5)
  assert(a.bounds.y == 6)
  assert(a.bounds.width == 7)
  assert(a.bounds.height == 8)

  a:deselect()
  assert(a.bounds.x == 0)
  assert(a.bounds.y == 0)
  assert(a.bounds.width == 0)
  assert(a.bounds.height == 0)
  assert(a.isEmpty)
  assert(not a:contains(0, 0))

  -- Constructor with rectangles
  local b = Selection(1, 2, 3, 4)
  assert(b.bounds == Rectangle(1, 2, 3, 4))
  assert(b.origin == Point(1, 2))

  -- Move
  b.origin = Point(5, 6)
  assert(b.bounds == Rectangle(5, 6, 3, 4))
  assert(b.origin == Point(5, 6))
end

-- Sprite Selection
do
  local spr = Sprite(32, 32)
  local sel = spr.selection
  assert(sel.bounds.x == 0)
  assert(sel.bounds.y == 0)
  assert(sel.bounds.width == 0)
  assert(sel.bounds.height == 0)

  sel:selectAll()
  assert(sel.bounds.x == 0)
  assert(sel.bounds.y == 0)
  assert(sel.bounds.width == spr.width)
  assert(sel.bounds.height == spr.height)

  sel:select(2, 3, 4, 5)
  assert(sel.bounds.x == 2)
  assert(sel.bounds.y == 3)
  assert(sel.bounds.width == 4)
  assert(sel.bounds.height == 5)

  sel.origin = Point(5, 6)
  assert(sel.bounds == Rectangle(5, 6, 4, 5))
end

-- Comparison
do
  local a = Selection()
  local b = Selection()
  assert(a == b)

  a:select(0, 0, 1, 1)
  assert(a ~= b)

  b:add(a)
  assert(a == b)

  a:subtract(b)
  assert(a ~= b)

  b:subtract(b)
  assert(a == b)
end

-- Operations
do
  local a = Selection()
  a:select(2, 3, 4, 5)
  assert(a.bounds == Rectangle(2, 3, 4, 5))

  a:subtract(2, 3, 4, 1)
  assert(a.bounds == Rectangle(2, 4, 4, 4))

  assert(a:contains(3, 5))
  a:subtract(3, 5, 1, 1)
  assert(not a:contains(3, 5))
  assert(a.bounds == Rectangle(2, 4, 4, 4))

  local b = Selection()
  assert(a.bounds == Rectangle(2, 4, 4, 4))
  assert(b.isEmpty)
  a:subtract(b) -- This should be a no-op because b is empty
  assert(a.bounds == Rectangle(2, 4, 4, 4))

  b:select(0, 0, 32, 32)
  assert(a ~= b)
  b:intersect(a)
  assert(a == b)
  assert(b.bounds == Rectangle(2, 4, 4, 4))
  assert(b.bounds == Rectangle(2, 4, 4, 4))
end
