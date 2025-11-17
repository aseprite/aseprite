-- Copyright (C) 2025  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  local s = Sprite(32, 64)
  local l = app.layer

  assert(l.isVisible)
  assert(s.undoHistory.undoSteps == 0)

  -- Without undo
  l.isVisible = false
  assert(not l.isVisible)
  assert(s.undoHistory.undoSteps == 0)

  -- With undo
  app.transaction(function() l.isVisible = true end)
  assert(l.isVisible)
  assert(s.undoHistory.undoSteps == 1)

  app.undo()
  assert(not l.isVisible)
  assert(s.undoHistory.undoSteps == 0)
  assert(s.undoHistory.redoSteps == 1)
end

do
  local s = Sprite(32, 64)
  local l = app.layer

  assert(l.isVisible)
  assert(not l.isContinuous)
  assert(l.isEditable)

  -- Without undo
  l.isContinuous = true
  assert(l.isContinuous)
  assert(s.undoHistory.undoSteps == 0)

  l.isEditable = false
  assert(not l.isEditable)
  assert(s.undoHistory.undoSteps == 0)

  -- With undo
  app.transaction(function()
      l.isVisible = false
      l.isContinuous = false
      l.isEditable = true
  end)
  assert(not l.isVisible)
  assert(not l.isContinuous)
  assert(l.isEditable)
  assert(s.undoHistory.undoSteps == 1)

  app.undo()
  assert(l.isVisible)
  assert(l.isContinuous)
  assert(not l.isEditable)
  assert(s.undoHistory.undoSteps == 0)
  assert(s.undoHistory.redoSteps == 1)
end

-- Groups
do
  local s = Sprite(32, 64)
  s:newGroup()
  local g = app.layer
  assert(g.isExpanded)
  assert(not g.isCollapsed)
  assert(s.undoHistory.undoSteps == 1) -- newGroup() action

  g.isCollapsed = true
  assert(not g.isExpanded)
  assert(g.isCollapsed)
  assert(s.undoHistory.undoSteps == 1)

  g.isExpanded = true
  assert(g.isExpanded)
  assert(not g.isCollapsed)
  assert(s.undoHistory.undoSteps == 1)

  app.transaction(function() g.isCollapsed = true end)
  assert(not g.isExpanded)
  assert(g.isCollapsed)
  assert(s.undoHistory.undoSteps == 2)

  app.undo()
  assert(g.isExpanded)
  assert(not g.isCollapsed)
  assert(s.undoHistory.undoSteps == 1)
end
