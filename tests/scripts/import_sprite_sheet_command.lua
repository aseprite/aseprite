-- Copyright (C) 2021  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

dofile('./test_utils.lua')

do
  local s = Sprite(8, 4, ColorMode.INDEXED)
  assert(#s.cels == 1)

  local i = s.cels[1].image
  array_to_pixels({ 0, 1, 2, 3, 3, 2, 1, 0,
                    1, 2, 3, 4, 4, 3, 2, 1,
                    1, 2, 3, 4, 4, 3, 2, 1,
                    0, 1, 2, 3, 3, 2, 1, 0 }, i)

  app.command.ImportSpriteSheet{
    ui=false,
    type=SpriteSheetType.ROWS,
    frameBounds=Rectangle(0, 0, 4, 4)
  }
  assert(#s.cels == 2)
  expect_img(s.cels[1].image,
             { 0, 1, 2, 3,
               1, 2, 3, 4,
               1, 2, 3, 4,
               0, 1, 2, 3 })
  expect_img(s.cels[2].image,
             { 3, 2, 1, 0,
               4, 3, 2, 1,
               4, 3, 2, 1,
               3, 2, 1, 0 })

  app.undo()
  app.command.ImportSpriteSheet{
    ui=false,
    type=SpriteSheetType.ROWS,
    frameBounds=Rectangle(0, 0, 2, 3)
  }
  assert(#s.cels == 4)
  expect_img(s.cels[1].image,
             { 0, 1,
               1, 2,
               1, 2 })
  expect_img(s.cels[2].image,
             { 2, 3,
               3, 4,
               3, 4 })
  expect_img(s.cels[3].image,
             { 3, 2,
               4, 3,
               4, 3 })
  expect_img(s.cels[4].image,
             { 1, 0,
               2, 1,
               2, 1 })


  app.undo()
  app.command.ImportSpriteSheet{
    ui=false,
    type=SpriteSheetType.ROWS,
    frameBounds=Rectangle(1, 1, 2, 2),
    padding=Size(2, 0)
  }
  assert(#s.cels == 2)
  expect_img(s.cels[1].image,
             { 2, 3,
               2, 3 })
  expect_img(s.cels[2].image,
             { 3, 2,
               3, 2 })

end
