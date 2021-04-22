-- Copyright (C) 2021  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

dofile('./test_utils.lua')

----------------------------------------------------------------------
-- Test magic wand in transparent layer
-- Note: A regression in the beta was found in this case.

do
  local s = Sprite(4, 4, ColorMode.INDEXED)
  app.command.LayerFromBackground()

  local i = s.cels[1].image
  i:clear(0)
  i:putPixel(0, 0, 1)
  expect_eq(4, i.width)
  expect_eq(4, i.height)

  app.useTool{ tool='magic_wand', points={Point(0, 0)} }
  expect_eq(Rectangle(0, 0, 1, 1), s.selection.bounds)

  app.useTool{ tool='magic_wand', points={Point(1, 0)} }
  expect_eq(Rectangle(0, 0, 4, 4), s.selection.bounds)
  assert(not s.selection:contains(0, 0))
end
