-- Copyright (C) 2022  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

dofile('./test_utils.lua')

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
