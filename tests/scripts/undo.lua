-- Copyright (C) 2026-present  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

dofile('./test_utils.lua')

-- Save undo history
do
  local spr = Sprite(2, 2, ColorMode.INDEXED)
  local lay = app.layer
  local cel = lay.cels[1]
  local img = cel.image

  -- Paint in cel
  app.useTool{ brush=1, color=1, points={{0,0},{1,1}} }
  app.useTool{ brush=1, color=2, points={{0,0}} }
  app.useTool{ brush=1, color=3, points={{1,0}} }

  -- Change cel properties
  expect_eq(Point(0, 0), cel.position)
  cel.position = Point(1, 2)

  expect_eq(255, cel.opacity)
  cel.opacity = 128

  expect_eq(0, cel.zIndex)
  cel.zIndex = -5

  -- Change layer properties
  expect_eq(255, lay.opacity)
  lay.opacity = 64

  -- New frame
  expect_eq(1, #spr.frames)
  app.command.NewFrame{ content="empty" }
  expect_eq(2, #spr.frames)

  -- New cel (painting)
  expect_eq(1, #lay.cels)
  app.useTool{ brush=1, color=3, points={{1,1}} }
  expect_eq(2, #lay.cels)

  -- New frame
  expect_eq(2, #spr.frames)
  app.command.NewFrame{ content="empty" }
  expect_eq(3, #spr.frames)

  -- New cel (painting)
  expect_eq(2, #lay.cels)
  app.useTool{ brush=1, color=4, points={{0,1}} }
  expect_eq(3, #lay.cels)
  expect_img(lay.cels[3].image, { 4 })

  -- Remove 2nd frame
  expect_eq(3, #spr.frames)
  app.frame = 2
  app.command.RemoveFrame()
  expect_eq(2, #spr.frames)

  -- Remove 1st frame
  expect_eq(2, #spr.frames)
  app.frame = 1
  app.command.RemoveFrame()
  expect_eq(1, #spr.frames)
  expect_eq(1, #lay.cels)
  expect_img(lay.cels[1].image, { 4 })

  app.command.SaveFileCopyAs{ filename="__test__undo1.aseprite",
                              ui=false,
                              saveUndoHistory=2 } -- external
end

function test_undoing()
  local spr = app.sprite
  local lay = app.layer

  expect_eq(1, #spr.frames)
  app.undo()
  expect_eq(2, #spr.frames)
  app.undo()
  expect_eq(3, #spr.frames)

  expect_eq(3, #lay.cels)
  app.undo()
  expect_eq(2, #lay.cels)

  expect_eq(3, #spr.frames)
  app.undo()
  expect_eq(2, #spr.frames)

  expect_eq(2, #lay.cels)
  app.undo()
  expect_eq(1, #lay.cels)

  expect_eq(2, #spr.frames)
  app.undo()
  expect_eq(1, #spr.frames)

  expect_eq(64, lay.opacity)
  app.undo()
  expect_eq(255, lay.opacity)

  local cel = lay.cels[1]
  local img = cel.image
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
end

function test_redoing()
  local spr = app.sprite
  local lay = app.layer
  local cel = lay.cels[1]
  local img = cel.image

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

  expect_eq(1, #spr.frames)
  app.redo()
  expect_eq(2, #spr.frames)

  expect_eq(1, #lay.cels)
  app.redo()
  expect_eq(2, #lay.cels)
  expect_img(lay.cels[2].image, { 3 })

  expect_eq(2, #spr.frames)
  app.redo()
  expect_eq(3, #spr.frames)

  expect_eq(2, #lay.cels)
  app.redo()
  expect_eq(3, #lay.cels)
  expect_img(lay.cels[3].image, { 4 })

  expect_eq(3, #spr.frames)
  app.redo()
  expect_eq(2, #spr.frames)
  app.redo()
  expect_eq(1, #spr.frames)
  expect_eq(1, #lay.cels)
  expect_img(lay.cels[1].image, { 4 })
end

-- Load with undo history
do
  local spr = Sprite{ fromFile="__test__undo1.aseprite" }
  test_undoing(spr)

  -- Save again (with new "saved state" in the first state)
  app.command.SaveFileCopyAs{ filename="__test__undo2.aseprite",
                              ui=false,
                              saveUndoHistory=2 } -- external
end

-- Reload the sprite with undo history, redo everything, undo
-- everything. Two times, to check that IDs are not duplicated
-- in-memory.
for i=1,2 do
  local spr = Sprite{ fromFile="__test__undo2.aseprite" }
  test_redoing(spr)
  test_undoing(spr)
end

-- assert(false)
