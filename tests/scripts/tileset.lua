-- Copyright (C) 2022-2023  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

dofile('./test_utils.lua')

-- Create a Tilemap and manipulate its Tileset
do
  local spr = Sprite(4, 4, ColorMode.INDEXED)

  app.command.NewLayer{ tilemap=true }
  local tilemap = spr.layers[2]

  local tileset = tilemap.tileset
  assert(#tileset == 1);

  app.useTool{
    tool='pencil',
    color=1,
    layer=tilemap,
    tilesetMode=TilesetMode.STACK,
    points={ Point(0, 0) }}
  assert(#tileset == 2)

  assert(tileset.name == "")
  tileset.name = "Default Land"
  assert(tileset.name == "Default Land")

  -- Tileset user data
  assert(tileset.data == "")
  tileset.data = "land"
  assert(tileset.data == "land")

  assert(tileset.color == Color())
  tileset.color = Color(255, 0, 0)
  assert(tileset.color == Color(255, 0, 0))

  -- Create extra tile
  app.useTool{
    tool='pencil',
    color=1,
    layer=tilemap,
    tilesetMode=TilesetMode.STACK,
    points={ Point(1, 1) }}
  assert(#tileset == 3)

  -- Check that Tileset:getTile(ti) returns Tileset:tile(ti).image user data
  for ti=0,2 do
    assert(tileset:tile(ti).image.id == tileset:getTile(ti).id)
  end
end

-- Create and delete Tilesets
do
  local spr = Sprite(4, 4, ColorMode.INDEXED)

  -- Create a tileset with default parameters
  local tileset = spr:newTileset()
  assert(#tileset == 1)
  assert(tileset.grid.origin.x == 0)
  assert(tileset.grid.origin.y == 0)
  assert(tileset.grid.tileSize.width == 16)
  assert(tileset.grid.tileSize.height == 16)
  assert(tileset.name == "")
  assert(#spr.tilesets == 1)

  -- Create a tileset passing a grid
  local tileset2 = spr:newTileset(Grid{0, 0 ,32, 32})
  assert(#tileset2 == 1)
  assert(tileset2.grid.origin.x == 0)
  assert(tileset2.grid.origin.y == 0)
  assert(tileset2.grid.tileSize.width == 32)
  assert(tileset2.grid.tileSize.height == 32)
  assert(tileset2.name == "")
  assert(#spr.tilesets == 2)

  -- Create a tileset passing a table and a number of tiles
  local tileset3 = spr:newTileset({0, 0 ,64, 64}, 5)
  assert(#tileset3 == 5)
  assert(tileset3.grid.origin.x == 0)
  assert(tileset3.grid.origin.y == 0)
  assert(tileset3.grid.tileSize.width == 64)
  assert(tileset3.grid.tileSize.height == 64)
  assert(tileset3.name == "")
  assert(#spr.tilesets == 3)
  tileset3.name = "Tileset 3"

  -- Duplicate a tileset
  local tileset4 = spr:newTileset(tileset3)
  assert(#tileset4 == 5)
  assert(tileset4.grid.origin.x == 0)
  assert(tileset4.grid.origin.y == 0)
  assert(tileset4.grid.tileSize.width == 64)
  assert(tileset4.grid.tileSize.height == 64)
  assert(tileset4.name == "Tileset 3")
  assert(#spr.tilesets == 4)

  -- Undo last tileset addition
  app.undo()
  assert(#spr.tilesets == 3)

  -- Delete tileset
  spr:deleteTileset(2)
  assert(#spr.tilesets == 2)
  spr:deleteTileset(tileset2)
  assert(#spr.tilesets == 1)

  -- Undo last tileset removal
  app.undo()
  assert(#spr.tilesets == 2)
  assert(#tileset2 == 1)
  assert(tileset2.grid.origin.x == 0)
  assert(tileset2.grid.origin.y == 0)
  assert(tileset2.grid.tileSize.width == 32)
  assert(tileset2.grid.tileSize.height == 32)
  assert(tileset2.name == "")
end
