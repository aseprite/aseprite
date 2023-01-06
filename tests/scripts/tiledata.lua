-- Copyright (C) 2023  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  local spr = Sprite(4, 4, ColorMode.INDEXED)
  spr.gridBounds = Rectangle(0, 0, 2, 2)
  app.command.NewLayer{ tilemap=true }
  local tilemap = spr.layers[2]
  local tileset = tilemap.tileset

  app.useTool{
    tool='pencil',
    color=1,
    layer=tilemap,
    tilesetMode=TilesetMode.STACK,
    points={ Point(0, 0), Point(4, 3) }}
  assert(#tileset == 3)

  -- Tileset properties
  tileset.properties.kind = "terrain"
  assert(tileset.properties.kind == "terrain")
  assert(#tileset.properties == 1)

  -- Tiles user data
  local tile1 = tileset:tile(1)
  tile1.color = Color(255, 0, 0)
  assert(tile1.color == Color(255, 0, 0))
  tile1.data = "solid"
  assert(tile1.data == "solid")
  tile1.data = nil
  assert(tile1.data == "")

  tile1.properties.center = Point(2, 2)
  tile1.properties("ext").pivot = Point(5, 5)
  assert(tile1.properties.center == Point(2, 2))
  assert(tile1.properties("ext").pivot == Point(5, 5))
  assert(#tileset.properties == 1) -- Check that tileset properties are not set

  local tile2 = tileset:tile(2)
  tile2.properties.center = Point(3, 2)
  tile2.properties.extra = 32
  assert(tile2.properties.center == Point(3, 2))
  assert(tile2.properties.extra == 32)
  assert(tile1.properties.center == Point(2, 2))
  assert(tile1.properties("ext").pivot == Point(5, 5))
  assert(#tileset.properties == 1) -- Check that tileset properties are not set
  assert(#tile1.properties == 1)
  assert(#tile1.properties("ext") == 1)
  assert(#tile2.properties == 2)
  app.undo()
  assert(#tile2.properties == 1)
  app.undo()
  assert(#tile2.properties == 0)
  app.redo()
  app.redo()
  assert(#tile2.properties == 2)

  -- Undoable Tile.color and Tile.data
  assert(tile2.color == Color())
  tile2.color = Color(0, 0, 255)
  assert(tile2.color == Color(0, 0, 255))
  app.undo()
  assert(tile2.color == Color())

  assert(tile2.data == "")
  tile2.data = "B"
  assert(tile2.data == "B")
  app.undo()
  assert(tile2.data == "")

  -- Set all properties at once
  tile1.properties = { a=1, b=2.1 }
  assert(#tile1.properties == 2)
  assert(tile1.properties.a == 1)
  assert(tile1.properties.b == 2.1)

  assert(#tile1.properties("ext") == 1)
  tile1.properties("ext", { x=2, y=3 })
  assert(#tile1.properties("ext") == 2)
  assert(tile1.properties("ext").x == 2)
  assert(tile1.properties("ext").y == 3)

end
