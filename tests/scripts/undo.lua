-- Copyright (C) 2026-present  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

dofile('./test_utils.lua')

-- Save undo history
do
  local spr = Sprite(2, 2, ColorMode.INDEXED)
  local lay = app.layer
  local cel = app.cel
  local img = app.image

  app.useTool{ brush=1, color=1, points={{0,0},{1,1}} }
  app.useTool{ brush=1, color=2, points={{0,0}} }
  app.useTool{ brush=1, color=3, points={{1,0}} }

  expect_eq(Point(0, 0), cel.position)
  cel.position = Point(1, 2)

  expect_eq(255, cel.opacity)
  cel.opacity = 128

  expect_eq(0, cel.zIndex)
  cel.zIndex = -5

  expect_eq(255, lay.opacity)
  lay.opacity = 64

  app.command.SaveFileCopyAs{ filename="__test__undo.aseprite",
                              ui=false,
                              saveUndoHistory=2 } -- external
end

-- Load with undo history
do
  local spr = Sprite{ fromFile="__test__undo.aseprite" }
  local lay = app.layer
  local cel = app.cel
  local img = app.image

  expect_eq(64, lay.opacity)
  app.undo()
  expect_eq(255, lay.opacity)

  expect_eq(-5, cel.zIndex)
  app.undo()
  expect_eq(0, cel.zIndex)

  expect_eq(128, cel.opacity)
  app.undo()
  expect_eq(255, cel.opacity)

  expect_eq(Point(1, 2), cel.position)
  app.undo()
  expect_eq(Point(0, 0), cel.position)

  expect_img(img, { 2, 3,
                    0, 1 })
  app.undo()
  expect_img(img, { 2, 0,
                    0, 1 })
  app.undo()
  expect_img(img, { 1, 0,
                    0, 1 })
  app.undo()
  expect_img(img, { 0, 0,
                    0, 0 })

  -- Save again (with new "saved state")
  app.command.SaveFileCopyAs{ filename="__test__undo.aseprite",
                              ui=false,
                              saveUndoHistory=2 } -- external
end

-- Reload time with undo history
do
  local spr = Sprite{ fromFile="__test__undo.aseprite" }
  local lay = app.layer
  local cel = app.cel
  local img = app.image

  expect_img(img, { 0, 0,
                    0, 0 })
  app.redo()
  expect_img(img, { 1, 0,
                    0, 1 })
  app.redo()
  expect_img(img, { 2, 0,
                    0, 1 })
  app.redo()
  expect_img(img, { 2, 3,
                    0, 1 })

  expect_eq(Point(0, 0), cel.position)
  app.redo()
  expect_eq(Point(1, 2), cel.position)

  expect_eq(255, cel.opacity)
  app.redo()
  expect_eq(128, cel.opacity)

  expect_eq(0, cel.zIndex)
  app.redo()
  expect_eq(-5, cel.zIndex)

  expect_eq(255, lay.opacity)
  app.redo()
  expect_eq(64, lay.opacity)
end
