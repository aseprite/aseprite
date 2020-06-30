-- Copyright (C) 2019-2020  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

-- This version of Aseprite doesn't support tilemaps
if TilesetMode == nil then return end

dofile('./test_utils.lua')

local rgba = app.pixelColor.rgba

-- Check constants
assert(TilemapMode.PIXELS == 0)
assert(TilemapMode.TILES == 1)
assert(TilesetMode.STACK == 2)
assert(TilesetMode.MANUAL == 0)
assert(TilesetMode.AUTO == 1)
assert(TilesetMode.STACK == 2)

----------------------------------------------------------------------
-- Tests drawing in the tilemap
----------------------------------------------------------------------

do
  local spr = Sprite(32, 32)
  assert(spr.layers[1].isImage)
  assert(not spr.layers[1].isTilemap)

  ----------------------------------------------------------------------
  -- Create a tilemap
  ----------------------------------------------------------------------

  app.command.NewLayer{ tilemap=true }
  assert(#spr.layers == 2)
  local tilemapLay = spr.layers[2]
  assert(tilemapLay.isImage)
  assert(tilemapLay.isTilemap)
  assert(#tilemapLay.cels == 0)

  ----------------------------------------------------------------------
  -- Draw the first pixel on the tilemap so a new tile is created
  ----------------------------------------------------------------------

  app.useTool{
    tool='pencil',
    color=Color{ r=0, g=0, b=0 },
    layer=tilemapLay,
    tilesetMode=TilesetMode.STACK,
    points={ Point(2, 2), Point(3, 2) }}
  assert(#tilemapLay.cels == 1)
  assert(tilemapLay:cel(1).image.colorMode == ColorMode.TILEMAP)
  local tilemapCel = tilemapLay:cel(1)
  assert(tilemapCel.bounds == Rectangle(0, 0, 16, 16))

  assert(#spr.tilesets == 1) -- one tileset
  assert(spr.tilesets[1] == tilemapLay.tileset)

  local tileset = tilemapLay.tileset
  assert(#tileset == 1) -- one tile

  tilemapCel.position = Point(2, 2)
  assert(tilemapCel.bounds == Rectangle(2, 2, 16, 16))
  assert(#tileset == 1)

  assert(tilemapCel.image.width == 1)
  assert(tilemapCel.image.height == 1)
  assert(tilemapCel.image:getPixel(0, 0) == 0)

  ----------------------------------------------------------------------
  -- Draw a second pixel with locked mode (new tiles are not generated)
  ----------------------------------------------------------------------

  app.useTool{
    tool='pencil',
    color=Color{ r=0, g=0, b=0 },
    cel=tilemapCel,
    tilesetMode=TilesetMode.MANUAL,
    points={ Point(0, 0) }}

  assert(tilemapCel.bounds == Rectangle(2, 2, 16, 16))
  assert(#tileset == 1)

  ----------------------------------------------------------------------
  -- Draw pixels generating new tiles
  ----------------------------------------------------------------------

  app.useTool{
    tool='pencil',
    color=Color{ r=0, g=0, b=0 },
    cel=tilemapCel,
    tilesetMode=TilesetMode.STACK,
    points={ Point(0, 0) }}

  assert(tilemapCel.bounds == Rectangle(-14, -14, 32, 32))
  assert(#tileset == 3)
  assert(not tileset:getTile(0):isEmpty())
  assert(not tileset:getTile(1):isEmpty())
  assert(tileset:getTile(2):isEmpty())
  assert(tilemapCel.image.width == 2)
  assert(tilemapCel.image.height == 2)
  assert(tilemapCel.image:getPixel(0, 0) == 1)
  assert(tilemapCel.image:getPixel(1, 0) == 2)
  assert(tilemapCel.image:getPixel(0, 1) == 2)
  assert(tilemapCel.image:getPixel(1, 1) == 0)

  app.useTool{
    tool='pencil',
    color=Color{ r=0, g=0, b=0 },
    cel=tilemapCel,
    tilesetMode=TilesetMode.STACK,
    points={ Point(19, 19) }}

  assert(tilemapCel.bounds == Rectangle(-14, -14, 48, 48))
  assert(#tilemapLay.tileset == 4)
  assert(not tileset:getTile(3):isEmpty())
  assert(tilemapCel.image.width == 3)
  assert(tilemapCel.image.height == 3)
  assert(tilemapCel.image:getPixel(0, 0) == 1)
  assert(tilemapCel.image:getPixel(1, 0) == 2)
  assert(tilemapCel.image:getPixel(2, 0) == 2)
  assert(tilemapCel.image:getPixel(0, 1) == 2)
  assert(tilemapCel.image:getPixel(1, 1) == 0)
  assert(tilemapCel.image:getPixel(2, 1) == 2)
  assert(tilemapCel.image:getPixel(0, 2) == 2)
  assert(tilemapCel.image:getPixel(1, 2) == 2)
  assert(tilemapCel.image:getPixel(2, 2) == 3)
end

----------------------------------------------------------------------
-- Tests drawing when the grid origin is in a negative position
----------------------------------------------------------------------

do
  local spr = Sprite(32, 32)
  app.command.NewLayer{ tilemap=true }
  local tilemapLay = spr.layers[2]

  app.useTool{
    tool='pencil',
    color=Color{ r=0, g=255, b=0 },
    layer=tilemapLay,
    tilesetMode=TilesetMode.STACK,
    points={ Point(0, 0) }}
  local tilemapCel = tilemapLay:cel(1)
  assert(tilemapCel.bounds == Rectangle(0, 0, 16, 16))

  local tileset = tilemapLay.tileset
  assert(#tileset == 1) -- one tile

  tilemapCel.position = Point(-1, -1)
  assert(tilemapCel.bounds == Rectangle(-1, -1, 16, 16))

  app.useTool{
    tool='pencil',
    color=Color{ r=255, g=0, b=0 },
    cel=tilemapCel,
    tilesetMode=TilesetMode.STACK,
    points={ Point(0, 0) }}

  assert(tilemapCel.bounds == Rectangle(-1, -1, 16, 16))
  assert(#tileset == 2)

  local img = tileset:getTile(1)

  assert(img:getPixel(0, 0) == rgba(0, 255, 0, 255))
  assert(img:getPixel(1, 0) == rgba(0, 0, 0, 0))
  assert(img:getPixel(2, 0) == rgba(0, 0, 0, 0))

  assert(img:getPixel(0, 1) == rgba(0, 0, 0, 0))
  assert(img:getPixel(1, 1) == rgba(255, 0, 0, 255))
  assert(img:getPixel(2, 1) == rgba(0, 0, 0, 0))

  assert(img:getPixel(0, 2) == rgba(0, 0, 0, 0))
  assert(img:getPixel(1, 2) == rgba(0, 0, 0, 0))
  assert(img:getPixel(2, 2) == rgba(0, 0, 0, 0))
end

----------------------------------------------------------------------
-- Tests that extra tiles are not created
----------------------------------------------------------------------

do
  local spr = Sprite(32, 32)
  app.command.NewLayer{ tilemap=true }
  local tilemapLay = spr.layers[2]

  app.useTool{
    tool='pencil',
    color=Color{ r=0, g=255, b=0 },
    layer=tilemapLay,
    tilesetMode=TilesetMode.STACK,
    points={ Point(0, 0) }}
  local tilemapCel = tilemapLay:cel(1)
  tilemapCel.position = Point(-1, -1)

  app.useTool{
    tool='pencil',
    color=Color{ r=255, g=0, b=0 },
    cel=tilemapCel,
    tilesetMode=TilesetMode.STACK,
    points={ Point(30, 30) }}

  assert(tilemapCel.bounds == Rectangle(-1, -1, 32, 32))
end

----------------------------------------------------------------------
-- Tests moving tiles
----------------------------------------------------------------------

do
  local spr = Sprite(32, 32)
  app.command.NewLayer{ tilemap=true }
  local tilemapLay = spr.layers[2]

  app.useTool{
    tool='ellipse',
    color=Color{ r=0, g=0, b=0 },
    layer=tilemapLay,
    tilesetMode=TilesetMode.STACK,
    points={ Point(0, 0), Point(31, 31) }}

  local ts = tilemapLay.tileset
  assert(#ts == 4)

  local imgs = { ts:getTile(0),
                 ts:getTile(1),
                 ts:getTile(2),
                 ts:getTile(3) }

  -- No op = move tile 0 before the tile 0, result=0,1,2,3
  app.range.tiles = { 0 }
  app.command.MoveTiles{ before=0 }
  expect_eq(imgs[0+1], ts:getTile(0))
  expect_eq(imgs[1+1], ts:getTile(1))
  expect_eq(imgs[2+1], ts:getTile(2))
  expect_eq(imgs[3+1], ts:getTile(3))

  -- Move tile 1 before tile 0, result=1,0,2,3
  app.range.tiles = { 1 }
  app.command.MoveTiles{ before=0 }
  expect_eq(imgs[1+1], ts:getTile(0))
  expect_eq(imgs[0+1], ts:getTile(1))
  expect_eq(imgs[2+1], ts:getTile(2))
  expect_eq(imgs[3+1], ts:getTile(3))
  app.undo()

  -- Move tiles 0 and 1 before 3, result=2,0,1,3
  app.range.tiles = { 0, 1 }
  app.command.MoveTiles({ before=3 })
  expect_eq(imgs[2+1], ts:getTile(0))
  expect_eq(imgs[0+1], ts:getTile(1))
  expect_eq(imgs[1+1], ts:getTile(2))
  expect_eq(imgs[3+1], ts:getTile(3))
  app.undo()

  -- Move tiles 0 and 2 before 2, result=1,0,2,3
  app.range.tiles = { 0, 2 }
  app.command.MoveTiles({ before=2 })
  expect_eq(imgs[1+1], ts:getTile(0))
  expect_eq(imgs[0+1], ts:getTile(1))
  expect_eq(imgs[2+1], ts:getTile(2))
  expect_eq(imgs[3+1], ts:getTile(3))
  app.undo()

end
