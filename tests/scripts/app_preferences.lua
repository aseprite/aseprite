-- Copyright (C) 2019-2021  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

-- Preferences for tools
do
  -- The first time we get the tool preferences in CLI mode, we get
  -- the default options for this tool (in GUI, we get the current
  -- user-defined options).
  local t = app.preferences.tool('pencil')
  assert(t.opacity == 255)
  assert(t.tolerance == 0)
  assert(t.contiguous == true)
  assert(t.brush.type == BrushType.CIRCLE)
  assert(t.brush.size == 1)
  t.brush.size = 2
  assert(t.brush.size == 2)

  -- Getting the tool again must give us the configuration that was
  -- set inside the script
  t = app.preferences.tool('pencil')
  assert(t.brush.size == 2)
end

-- Preferences for documents
do
  local defPref = app.preferences.document(nil)
  defPref.grid.bounds = Rectangle(0, 0, 16, 16)

  local s = Sprite(32, 32)
  local p = app.preferences.document(s)
  assert(s.gridBounds == Rectangle(0, 0, 16, 16))
  assert(p.grid.bounds == Rectangle(0, 0, 16, 16))
  assert(p.grid.color == Color(0, 0, 255))
  p.grid.color = Color(255, 0, 0)
  assert(p.grid.color == Color(255, 0, 0))
end

-- Default & doc preferences combinations (related to https://community.aseprite.org/t/4305)
do
  -- Set default preferences
  local defPref = app.preferences.document(nil)
  defPref.grid.bounds = Rectangle(0, 0, 10, 10)
  defPref.grid.color = Color(255, 255, 0) -- Yellow
  assert(defPref.grid.bounds == Rectangle(0, 0, 10, 10))
  assert(defPref.grid.color == Color(255, 255, 0))

  do -- File with default preferences
    local doc = Sprite(32, 32)
    local docPref = app.preferences.document(doc)
    assert(doc.gridBounds == Rectangle(0, 0, 10, 10))
    assert(docPref.grid.bounds == Rectangle(0, 0, 10, 10))
    assert(docPref.grid.color == Color(255, 255, 0))
    doc:saveAs('_test_pref.png')
    doc:close()
  end

  do -- File with specific preferences
    local doc = Sprite(32, 32)
    local docPref = app.preferences.document(doc)
    docPref.grid.color = Color(0, 128, 0) -- Green
    assert(docPref.grid.bounds == Rectangle(0, 0, 10, 10))
    assert(docPref.grid.color == Color(0, 128, 0))
    doc:saveAs('_test_pref2.png')
    doc:close()
  end

  -- Now we change default preferences
  defPref.grid.bounds = Rectangle(0, 0, 64, 64)
  defPref.grid.color = Color(255, 0, 0) -- Red
  assert(defPref.grid.bounds == Rectangle(0, 0, 64, 64))
  assert(defPref.grid.color == Color(255, 0, 0))
  do -- A new document should have the new default preferences
    local doc = Sprite(32, 32)
    local docPref = app.preferences.document(doc)
    assert(docPref.grid.bounds == Rectangle(0, 0, 64, 64))
    assert(docPref.grid.color == Color(255, 0, 0))
    doc:close()
  end
  do -- The first document should have the new default preferences (because it was saved with the defaults)

    -- TODO maybe related to https://community.aseprite.org/t/grid-for-new-documents/3303
    --      should we get the new defaults or the values when we saved the document?
    --      (e.g. even if we didn't change the grid values, it looks
    --      like if the user changed something about the grid or the
    --      background colors, the expected behavior would be to
    --      restore the exact same colors, no the new defaults)

    local doc = Sprite{ fromFile="_test_pref.png" }
    local docPref = app.preferences.document(doc)
    assert(docPref.grid.bounds == Rectangle(0, 0, 64, 64))
    assert(docPref.grid.color == Color(255, 0, 0))
    doc:close()
  end
  do -- The second document should have the new specific preferences for the grid color
    local doc = Sprite{ fromFile="_test_pref2.png" }
    local docPref = app.preferences.document(doc)
    assert(docPref.grid.bounds == Rectangle(0, 0, 64, 64))
    assert(docPref.grid.color == Color(0, 128, 0))
    doc:close()
  end
end

do -- Test that undoing grid bounds, updates the grid in the preferences correctly
  local doc = Sprite(32, 32)
  assert(doc.gridBounds == Rectangle(0, 0, 64, 64))
  assert(app.preferences.document(doc).grid.bounds == Rectangle(0, 0, 64, 64))

  doc.gridBounds = Rectangle(0, 0, 4, 4)
  assert(doc.gridBounds == Rectangle(0, 0, 4, 4))
  assert(app.preferences.document(doc).grid.bounds == Rectangle(0, 0, 4, 4))

  app.undo()
  assert(doc.gridBounds == Rectangle(0, 0, 64, 64))
  assert(app.preferences.document(doc).grid.bounds == Rectangle(0, 0, 64, 64))

  app.redo()
  assert(doc.gridBounds == Rectangle(0, 0, 4, 4))
  assert(app.preferences.document(doc).grid.bounds == Rectangle(0, 0, 4, 4))
end

do -- Test symmetry preferences
  do -- File with default preferences
    local doc = Sprite(200, 100)
    local docPref = app.preferences.document(doc)
    assert(docPref.symmetry.x_axis == 100)
    assert(docPref.symmetry.y_axis == 50)
    docPref.symmetry.x_axis = 25
    doc:saveAs('_test_symmetry.png')
    doc:close()
  end
  do -- File with default preferences
    local doc = Sprite{ fromFile='_test_symmetry.png' }
    local docPref = app.preferences.document(doc)
    assert(docPref.symmetry.x_axis == 25)
    assert(docPref.symmetry.y_axis == 50)
    doc:close()
  end
end
