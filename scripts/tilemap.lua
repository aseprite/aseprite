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
  expect_img(tilemapCel.image, { 1, 2,
                                 2, 0 })

  app.useTool{
    tool='pencil',
    color=Color{ r=0, g=0, b=0 },
    cel=tilemapCel,
    tilesetMode=TilesetMode.STACK,
    points={ Point(19, 19) }}

  assert(tilemapCel.bounds == Rectangle(-14, -14, 48, 48))
  assert(#tilemapLay.tileset == 4)
  assert(not tileset:getTile(3):isEmpty())
  expect_img(tilemapCel.image, { 1, 2, 2,
                                 2, 0, 2,
                                 2, 2, 3 })
end

----------------------------------------------------------------------
-- Tests drawing in the tilemap with tiles
----------------------------------------------------------------------

do
  local spr = Sprite(32, 32)
  spr.gridBounds = Rectangle(0, 0, 8, 8)
  app.command.NewLayer{ tilemap=true }

  local tm = app.activeLayer
  local ts = tm.tileset
  assert(ts ~= nil)
  expect_eq(0, ts.grid.origin.x)
  expect_eq(0, ts.grid.origin.y)
  expect_eq(8, ts.grid.tileSize.width)
  expect_eq(8, ts.grid.tileSize.height)

  app.useTool{
    tool='pencil',
    color=Color{ r=0, g=0, b=0 },
    tilemapMode=TilesetMode.PIXELS,
    tilesetMode=TilesetMode.STACK,
    points={ Point(0, 0), Point(31, 31) }}

  local cel = tm.cels[1]
  expect_eq(2, #ts)
  expect_img(cel.image, { 0,1,1,1,
                          1,0,1,1,
                          1,1,0,1,
                          1,1,1,0 })

  app.useTool{
    tool='pencil',
    color=Color{ index=0 },
    tilemapMode=TilemapMode.TILES,
    tilesetMode=TilesetMode.STACK,
    points={ Point(0, 16) }} -- y=16 is the first pixel of 3rd row of tiles
  cel = tm.cels[1]
  expect_img(cel.image, { 0,1,1,1,
                          1,0,1,1,
                          0,1,0,1,
                          1,1,1,0 })

  app.useTool{
    tool='pencil',
    color=Color{ index=0 },
    tilemapMode=TilemapMode.TILES,
    tilesetMode=TilesetMode.STACK,
    points={ Point(0, 0), Point(16, 0) }} -- x=16 is the first pixel of 3rd column of tiles
  cel = tm.cels[1]
  expect_img(cel.image, { 0,0,0,1,
                          1,0,1,1,
                          0,1,0,1,
                          1,1,1,0 })

  -- Move layer origin to 10, 8
  cel.position = { 10, 8 }
  expect_eq(Point{ 10, 8 }, cel.position)
  app.useTool{
    tool='pencil',
    color=Color{ index=1 },
    tilemapMode=TilemapMode.TILES,
    tilesetMode=TilesetMode.STACK,
    points={ { 10, 8 }, { 18, 16 } }} -- {10,8} is the first existent tile in the tilemap
                                      -- these are tiles 2,1 and 3,2
  cel = tm.cels[1]
  expect_img(cel.image, { 1,0,0,1,
                          1,1,1,1,
                          0,1,0,1,
                          1,1,1,0 })

  app.useTool{
    tool='pencil',
    color=Color{ index=0 },
    tilemapMode=TilemapMode.TILES,
    tilesetMode=TilesetMode.STACK,
    points={ Point(1, 7), Point(2, 8) }} -- Tile 0,0 and 1,1

  -- TODO add a constant for the empty tile
  local e = tonumber("ffffffff", 16) -- empty tile (0xffffffff)

  cel = tm.cels[1]
  expect_img(cel.image, { 0,e, e,e,e,e,
                          e,0, 1,0,0,1,
                          e,e, 1,1,1,1,
                          e,e, 0,1,0,1,
                          e,e, 1,1,1,0 })

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
-- Tests moving & copying tiles
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
  expect_eq(1, #app.range.tiles)
  expect_eq(0, app.range.tiles[1])

  -- Move tile 1 before tile 0, result=1,0,2,3
  app.range.tiles = { 1 }
  app.command.MoveTiles{ before=0 }
  expect_eq(imgs[1+1], ts:getTile(0))
  expect_eq(imgs[0+1], ts:getTile(1))
  expect_eq(imgs[2+1], ts:getTile(2))
  expect_eq(imgs[3+1], ts:getTile(3))
  expect_eq(1, #app.range.tiles)
  expect_eq(0, app.range.tiles[1])
  app.undo()

  -- Move tiles 0 and 1 before 3, result=2,0,1,3
  app.range.tiles = { 0, 1 }
  app.command.MoveTiles({ before=3 })
  expect_eq(imgs[2+1], ts:getTile(0))
  expect_eq(imgs[0+1], ts:getTile(1))
  expect_eq(imgs[1+1], ts:getTile(2))
  expect_eq(imgs[3+1], ts:getTile(3))
  expect_eq(2, #app.range.tiles)
  expect_eq(1, app.range.tiles[1])
  expect_eq(2, app.range.tiles[2])
  app.undo()

  -- Move tiles 0 and 2 before 2, result=1,0,2,3
  app.range.tiles = { 0, 2 }
  app.command.MoveTiles({ before=2 })
  expect_eq(imgs[1+1], ts:getTile(0))
  expect_eq(imgs[0+1], ts:getTile(1))
  expect_eq(imgs[2+1], ts:getTile(2))
  expect_eq(imgs[3+1], ts:getTile(3))
  expect_eq(2, #app.range.tiles)
  expect_eq(1, app.range.tiles[1])
  expect_eq(2, app.range.tiles[2])
  app.undo()

  -- Copy tiles 0 before 0, result=0,0,1,2,3
  app.range.tiles = { 0 }
  app.command.CopyTiles({ before=0 })
  expect_eq(1, #app.range.tiles)
  expect_eq(0, app.range.tiles[1])
  expect_eq(5, #ts)
  assert(ts:getTile(0):isEqual(imgs[0+1]))
  assert(ts:getTile(1):isEqual(imgs[0+1]))
  assert(ts:getTile(2):isEqual(imgs[1+1]))
  assert(ts:getTile(3):isEqual(imgs[2+1]))
  assert(ts:getTile(4):isEqual(imgs[3+1]))
  app.undo()

  -- Copy tiles 0 before 3, result=0,1,2,0,3
  app.range.tiles = { 0 }
  app.command.CopyTiles({ before=3 })
  expect_eq(1, #app.range.tiles)
  expect_eq(3, app.range.tiles[1])
  expect_eq(5, #ts)
  assert(ts:getTile(0):isEqual(imgs[0+1]))
  assert(ts:getTile(1):isEqual(imgs[1+1]))
  assert(ts:getTile(2):isEqual(imgs[2+1]))
  assert(ts:getTile(3):isEqual(imgs[0+1]))
  assert(ts:getTile(4):isEqual(imgs[3+1]))
  app.undo()

  -- Copy tiles 0, 1, and 3, before 4, result=0,1,2,3,0,1,3
  app.range.tiles = { 0, 1, 3 }
  app.command.CopyTiles({ before=4 })
  assert(ts:getTile(0):isEqual(imgs[0+1]))
  assert(ts:getTile(1):isEqual(imgs[1+1]))
  assert(ts:getTile(2):isEqual(imgs[2+1]))
  assert(ts:getTile(3):isEqual(imgs[3+1]))
  assert(ts:getTile(4):isEqual(imgs[0+1]))
  assert(ts:getTile(5):isEqual(imgs[1+1]))
  assert(ts:getTile(6):isEqual(imgs[3+1]))
  app.undo()

  -- Copy tiles 1, and 3, before 0, result=1,3,0,1,2,3
  app.range.tiles = { 1, 3 }
  app.command.CopyTiles({ before=0 })
  assert(ts:getTile(0):isEqual(imgs[1+1]))
  assert(ts:getTile(1):isEqual(imgs[3+1]))
  assert(ts:getTile(2):isEqual(imgs[0+1]))
  assert(ts:getTile(3):isEqual(imgs[1+1]))
  assert(ts:getTile(4):isEqual(imgs[2+1]))
  assert(ts:getTile(5):isEqual(imgs[3+1]))
  app.undo()

  -- Copy tiles 0, and 3, before 7, result=0,1,2,3,0,3
  app.range.tiles = { 0, 3 }
  app.command.CopyTiles({ before=4 })
  assert(ts:getTile(0):isEqual(imgs[0+1]))
  assert(ts:getTile(1):isEqual(imgs[1+1]))
  assert(ts:getTile(2):isEqual(imgs[2+1]))
  assert(ts:getTile(3):isEqual(imgs[3+1]))
  assert(ts:getTile(4):isEqual(imgs[0+1]))
  assert(ts:getTile(5):isEqual(imgs[3+1]))
  app.undo()

  -- Copy tiles 0, and 3, before 7, result=0,1,2,3,-,0,3
  app.range.tiles = { 0, 3 }
  app.command.CopyTiles({ before=5 })
  assert(ts:getTile(0):isEqual(imgs[0+1]))
  assert(ts:getTile(1):isEqual(imgs[1+1]))
  assert(ts:getTile(2):isEqual(imgs[2+1]))
  assert(ts:getTile(3):isEqual(imgs[3+1]))
  assert(ts:getTile(4):isPlain())
  assert(ts:getTile(5):isEqual(imgs[0+1]))
  assert(ts:getTile(6):isEqual(imgs[3+1]))
  app.undo()

  -- Copy tiles 2, and 3, before 7, result=0,1,2,3,-,-,-,2,3
  app.range.tiles = { 2, 3 }
  app.command.CopyTiles({ before=7 })
  assert(ts:getTile(0):isEqual(imgs[0+1]))
  assert(ts:getTile(1):isEqual(imgs[1+1]))
  assert(ts:getTile(2):isEqual(imgs[2+1]))
  assert(ts:getTile(3):isEqual(imgs[3+1]))
  assert(ts:getTile(4):isPlain())
  assert(ts:getTile(5):isPlain())
  assert(ts:getTile(6):isPlain())
  assert(ts:getTile(7):isEqual(imgs[2+1]))
  assert(ts:getTile(8):isEqual(imgs[3+1]))
  app.undo()

end
