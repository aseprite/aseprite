-- Copyright (C) 2026-present  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

dofile('./test_utils.lua')

-- Basic example keeping an empty cel with user data
do
  local s = Sprite(4, 4, ColorMode.INDEXED)
  expect_img(s.cels[1].image, { 0, 0, 0, 0,
                                0, 0, 0, 0,
                                0, 0, 0, 0,
                                0, 0, 0, 0 })

  app.useTool{ color=1, brush=1, tool="pencil", points={ {1,1} } }
  expect_img(s.cels[1].image, { 1 })

  -- Empty cel is destroyed after erasing its only pixel
  app.useTool{ color=0, brush=1, tool="pencil", points={ {1,1} } }
  assert(#s.cels == 0)
  assert(app.cel == nil)

  -- Re-create cel and check that a cel with user data is not destroyed
  app.useTool{ color=2, brush=1, tool="pencil", points={ {1,1} } }
  expect_img(s.cels[1].image, { 2 })
  assert(app.cel ~= nil)
  assert(app.cel.image ~= nil)
  app.cel.data = "keepalive"
  app.useTool{ color=0, brush=1, tool="pencil", points={ {1,1} } }
  assert(#s.cels == 1)
  assert(app.cel ~= nil)
  assert(app.cel.image == nil)
  assert(app.cel.data == "keepalive")
end

-- FlattenLayers with empty cels
do
  local s = Sprite(4, 4, ColorMode.INDEXED)
  expect_img(app.cel.image, { 0, 0, 0, 0,
                              0, 0, 0, 0,
                              0, 0, 0, 0,
                              0, 0, 0, 0 })

  app.useTool{ color=1, brush=1, tool="filled_rectangle", points={ {1,1}, {2,2} } }
  expect_img(app.cel.image, { 1, 1,
                              1, 1 })

  app.command.NewLayer()
  app.useTool{ color=2, brush=1, tool="pencil", points={ {2,2} } }
  expect_img(app.cel.image, { 2 })
  app.cel.zIndex = 1

  -- Regular FlattenLayers
  app.command.FlattenLayers()
  expect_img(app.cel.image, { 1, 1,
                              1, 2 })

  app.undo()

  -- Erase the front layer image
  app.layer = s.layers[2]
  app.useTool{ brush=1, tool="eraser", points={ {2,2} } }
  assert(app.cel.image == nil)
  assert(app.cel.zIndex == 1)
  assert(app.cel == s.cels[2])

  -- Flatten empty cel to a regular image
  app.command.FlattenLayers()
  assert(#s.cels == 1)
  expect_img(s.cels[1].image, { 1, 1,
                                1, 1 })
end
