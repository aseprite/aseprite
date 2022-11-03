-- Copyright (C) 2022  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local fn = '_test_modified.png'

do
  local doc = Sprite(2, 2)

  -- New sprites are created without modifications (can be closed
  -- without warning)
  assert(not doc.isModified)

  doc.width = 3
  assert(doc.isModified)
  app.undo()
  assert(not doc.isModified)
  app.redo()
  assert(doc.isModified)

  -- Not modified after it's saved
  doc:saveAs(fn)
  assert(not doc.isModified)

  -- Modified if we undo the saved state
  app.undo()
  assert(doc.width == 2)
  assert(doc.isModified)
  app.redo()
  assert(doc.width == 3)
  assert(not doc.isModified)

  -- Selection changes shouldn't change the modified flag
  app.command.MaskAll()
  assert(not doc.isModified)
  app.command.DeselectMask()
  assert(not doc.isModified)
  doc:saveAs(fn)
  assert(not doc.isModified)
  app.undo()                    -- Undo Deselect
  assert(not doc.isModified)
  app.undo()                    -- Undo Select All
  assert(not doc.isModified)
  assert(doc.width == 3)
  app.undo()                    -- Undo size change
  assert(doc.isModified)
  assert(doc.width == 2)
end

do
  local doc = Sprite{ fromFile=fn }

  -- Loaded sprites are created without modifications (can be closed
  -- without warning)
  assert(not doc.isModified)

  app.command.MaskAll()
  assert(not doc.isModified)
  doc:saveAs(fn)
  assert(not doc.isModified)
  app.undo()
  assert(not doc.isModified)
  app.redo()
  assert(not doc.isModified)
end
