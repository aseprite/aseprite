-- Copyright (C) 2024  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

dofile('./test_utils.lua')

do
  local sprite = Sprite{ fromFile="sprites/cut_paste.aseprite" }

  app.layer = sprite.layers[1]
  app.useTool {
    tool = "rectangular_marquee",
    points = {Point(0,1), Point(4,1)},
    selection = SelectionMode.REPLACE
  }
  app.command.Cut()
  sprite:newLayer()
  app.command.Paste()

  app.layer = sprite.layers[1]
  assert(app.cel.position == Point(1, 2))
  expect_img(app.activeImage,
            { 1, 1 })
  app.layer = sprite.layers[2]
  assert(app.cel.position == Point(2, 2))
  expect_img(app.activeImage,
            { 2, 2,
              2, 2 })

  app.layer = sprite.layers[3]
  assert(app.cel.position == Point(1, 1))
  expect_img(app.activeImage,
            { 1, 1 })
  app.command.FlattenLayers()
  assert(app.cel.position == Point(1, 1))
  expect_img(app.activeImage,
            { 1, 1, 0,
              1, 2, 2,
              0, 2, 2 })

  app.undo() -- Flatten
  app.undo() -- Paste
  app.undo() -- New Layer
  app.undo() -- Cut

  app.layer = sprite.layers[1]
  assert(app.cel.position == Point(1, 1))
  expect_img(app.activeImage,
            { 1, 1,
              1, 1 })
  app.layer = sprite.layers[2]
  assert(app.cel.position == Point(2, 2))
  expect_img(app.activeImage,
            { 2, 2,
              2, 2 })
  assert(#sprite.layers == 2)

  -- Another test
  app.layer = sprite.layers[1]
  app.useTool {
    tool = "rectangular_marquee",
    points = {Point(2,2), Point(4,2)},
    selection = SelectionMode.REPLACE
  }

  app.command.Cut()
  assert(app.cel.position == Point(1, 1))
  expect_img(app.activeImage,
            { 1, 1,
              1, 0 })
  sprite:newLayer()
  app.command.Paste()

  app.layer = sprite.layers[3]
  assert(app.cel.position == Point(2, 2))
  expect_img(app.activeImage,
            { 1 })

  app.undo() -- Paste
  app.undo() -- New Layer
  app.undo() -- Cut

  -- Test app.command.Copy
  app.layer = sprite.layers[1]
  app.useTool {
    tool = "rectangular_marquee",
    points = {Point(0,1), Point(4,1)},
    selection = SelectionMode.REPLACE
  }
  app.command.Copy()
  sprite:newLayer()
  app.command.Paste()

  app.layer = sprite.layers[1]
  assert(app.cel.position == Point(1, 1))
  expect_img(app.activeImage,
            { 1, 1,
              1, 1})
  app.layer = sprite.layers[2]
  assert(app.cel.position == Point(2, 2))
  expect_img(app.activeImage,
            { 2, 2,
              2, 2 })
  app.layer = sprite.layers[3]
  assert(app.cel.position == Point(1, 1))
  expect_img(app.activeImage,
            { 1, 1 })

  app.command.FlattenLayers()
  assert(app.cel.position == Point(1, 1))
  expect_img(app.activeImage,
            { 1, 1, 0,
              1, 2, 2,
              0, 2, 2 })

  -- Test app.command.Clear()
  app.useTool {
    tool = "rectangular_marquee",
    points = {Point(2,2), Point(4,2)},
    selection = SelectionMode.REPLACE
  }
  app.command.Clear()
  expect_img(app.activeImage,
            { 1, 1, 0,
              1, 0, 0,
              0, 2, 2 })

  app.useTool {
    tool = "rectangular_marquee",
    points = {Point(0,1), Point(4,2)},
    selection = SelectionMode.REPLACE
  }
  app.command.Clear()
  assert(app.cel.position == Point(2, 3))
  expect_img(app.activeImage,
            { 2, 2 })

  app.undo()
  app.undo()
  app.undo()

  assert(app.cel.position == Point(1, 1))
  expect_img(app.activeImage,
            { 1, 1, 0,
              1, 2, 2,
              0, 2, 2 })

  -- Test app.command.Cancel()
  app.useTool {
    tool = "rectangular_marquee",
    points = {Point(2,2), Point(4,2)},
    selection = SelectionMode.REPLACE
  }
  app.command.Cancel()
  app.command.Cut()
  assert(app.cel.position == Point(1, 1))
  expect_img(app.activeImage,
            { 1, 1, 0,
              1, 2, 2,
              0, 2, 2 })

  app.useTool {
    tool = "rectangular_marquee",
    points = {Point(2,0), Point(4,1)},
    selection = SelectionMode.REPLACE
  }
  app.command.Copy()
  sprite:newLayer()
  app.command.Paste { x=3, y=3 }
  app.command.FlattenLayers()
  assert(app.cel.position == Point(1, 1))
  expect_img(app.activeImage,
            { 1, 1, 0,
              1, 2, 2,
              0, 2, 2,
              0, 0, 1, })
end
