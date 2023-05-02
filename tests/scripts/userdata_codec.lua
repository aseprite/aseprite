-- Copyright (C) 2023  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

--
-- Codification and decodification of aseprite files with user data properties.
--
dofile('./test_utils.lua')

do
  local spr = Sprite{ fromFile="sprites/file-tests-props.aseprite" }

  -- Set sprite custom properties
  spr.properties.a = true
  spr.properties.b = 1
  spr.properties.c = "hi"
  spr.properties.d = 2.3
  spr.properties("ext").a = {"one", "two", "three"}
  -- Set layer custom properties
  spr.layers[2].properties.a = "i'm the layer 3"
  spr.layers[2].properties("ext").a = 100
  -- Set tileset custom properties
  spr.layers[1].tileset.properties.a = "i'm a tilemap"
  spr.layers[1].tileset.properties.b = 11
  spr.layers[1].tileset.properties("ext").a = "text from extension"
  -- Create some tiles with properties
  local tile = spr:newTile(spr.layers[1].tileset, 1)
  tile.properties.a = 320
  tile.properties.b = 330
  tile = spr:newTile(spr.layers[1].tileset, 2)
  tile.properties.a = 640
  tile.properties.b = 650
  -- Set tags custom properties
  spr.tags[1].properties.a = Point(1,2)
  spr.tags[1].properties.b = {a="text",b=35.567,c=Rectangle(1,2,3,4)}
  spr.tags[1].properties("ext").a = {a=Size(50,60),b=43985943434}
  -- Set cels custom properties
  spr.layers[4].cels[4].properties = {a={1,2,3}, b={a="4",b="5",c="6"}}
  spr.layers[4].cels[4].properties("ext").a = {"a","b","c"}
  -- Set slices custom properties
  spr.slices[1].properties = {a=Point(3,4), b=Size(10,20)}
  spr.slices[1].properties("ext", {a=Rectangle(10,20,30,40)})

  spr:saveAs("_test_userdata_codec_1.aseprite")
  spr:close()

  local origSpr = Sprite{ fromFile="sprites/file-tests-props.aseprite" }
  spr = Sprite{ fromFile="_test_userdata_codec_1.aseprite" }
  assert_sprites_eq(origSpr, spr)
  origSpr:close()
  assert(#spr.properties == 4)
  assert(#spr.properties("ext") == 1)
  assert(spr.properties.a == true)
  assert(spr.properties.b == 1)
  assert(spr.properties.c == "hi")
  assert(spr.properties.d == 2.3)
  assert(spr.properties("ext").a[1] == "one")
  assert(spr.properties("ext").a[2] == "two")
  assert(spr.properties("ext").a[3] == "three")
  assert(#spr.layers[2].properties == 1)
  assert(#spr.layers[2].properties("ext") == 1)
  assert(spr.layers[2].properties.a == "i'm the layer 3")
  assert(spr.layers[2].properties("ext").a == 100)
  assert(#spr.layers[1].tileset.properties == 2)
  assert(#spr.layers[1].tileset.properties("ext") == 1)
  assert(spr.layers[1].tileset.properties.a == "i'm a tilemap")
  assert(spr.layers[1].tileset.properties.b == 11)
  assert(spr.layers[1].tileset.properties("ext").a == "text from extension")
  assert(spr.layers[1].tileset:tile(1).properties.a == 320)
  assert(spr.layers[1].tileset:tile(1).properties.b == 330)
  assert(spr.layers[1].tileset:tile(2).properties.a == 640)
  assert(spr.layers[1].tileset:tile(2).properties.b == 650)
  assert(#spr.tags[1].properties == 2)
  assert(#spr.tags[1].properties("ext") == 1)
  assert(spr.tags[1].properties.a.x == 1)
  assert(spr.tags[1].properties.a.y == 2)
  assert(spr.tags[1].properties.b.a == "text")
  assert(spr.tags[1].properties.b.b == 35.567)
  assert(spr.tags[1].properties.b.c.x == 1)
  assert(spr.tags[1].properties.b.c.y == 2)
  assert(spr.tags[1].properties.b.c.width == 3)
  assert(spr.tags[1].properties.b.c.height == 4)
  assert(spr.tags[1].properties("ext").a.a.width == 50)
  assert(spr.tags[1].properties("ext").a.a.height == 60)
  assert(spr.tags[1].properties("ext").a.b == 43985943434)
  assert(#spr.layers[4].cels[4].properties == 2)
  assert(#spr.layers[4].cels[4].properties("ext") == 1)
  assert(spr.layers[4].cels[4].properties.a[1] == 1)
  assert(spr.layers[4].cels[4].properties.a[2] == 2)
  assert(spr.layers[4].cels[4].properties.a[3] == 3)
  assert(spr.layers[4].cels[4].properties.b.a == "4")
  assert(spr.layers[4].cels[4].properties.b.b == "5")
  assert(spr.layers[4].cels[4].properties.b.c == "6")
  assert(spr.layers[4].cels[4].properties("ext").a[1] == "a")
  assert(spr.layers[4].cels[4].properties("ext").a[2] == "b")
  assert(spr.layers[4].cels[4].properties("ext").a[3] == "c")
  assert(#spr.slices[1].properties == 2)
  assert(#spr.slices[1].properties("ext") == 1)
  assert(spr.slices[1].properties.a.x == 3)
  assert(spr.slices[1].properties.a.y == 4)
  assert(spr.slices[1].properties.b.width == 10)
  assert(spr.slices[1].properties.b.height == 20)
  assert(spr.slices[1].properties("ext").a.x == 10)
  assert(spr.slices[1].properties("ext").a.y == 20)
  assert(spr.slices[1].properties("ext").a.width == 30)
  assert(spr.slices[1].properties("ext").a.height == 40)
end

-- Test bug saving empty properties map with a empty set of properties
do
  local spr = Sprite(32, 32)
  spr:newLayer("B")

  -- Just create an empty property
  local properties1 = spr.layers[1].properties("ext")
  if properties1.categories then
    -- do nothing
  end

  local properties2 = spr.layers[2].properties("ext")
  properties2.b = 32

  spr:saveAs("_test_userdata_codec_2.aseprite")
  assert(#spr.layers == 2)
  spr:close()

  spr = Sprite{ fromFile="_test_userdata_codec_2.aseprite" }
  assert(#spr.layers[1].properties("ext") == 0)
  assert(#spr.layers[2].properties("ext") == 1)
  assert(spr.layers[2].properties("ext").b == 32)
end

-- Test save vector with different types inside
do
  local spr = Sprite(32, 32)
  spr.properties.a = { 3, "hi", {4, 5, 6}, {a="bye", b=10} }

  spr:saveAs("_test_userdata_codec_3.aseprite")
  spr:close()

  spr = Sprite{ fromFile="_test_userdata_codec_3.aseprite" }
  assert(#spr.properties.a == 4)
  assert(spr.properties.a[1] == 3)
  assert(spr.properties.a[2] == "hi")
  assert(#spr.properties.a[3] == 3)
  assert(spr.properties.a[3][1] == 4)
  assert(spr.properties.a[3][2] == 5)
  assert(spr.properties.a[3][3] == 6)
  assert(spr.properties.a[4].a == "bye")
  assert(spr.properties.a[4].b == 10)
end

-- Test save UUID
do
  local a = Uuid()
  local b = Uuid()
  local spr = Sprite(1, 1)
  spr.properties.a = a
  spr.properties.b = b

  spr:saveAs("_test_userdata_codec_4.aseprite")
  spr:close()

  spr = Sprite{ fromFile="_test_userdata_codec_4.aseprite" }
  assert(#spr.properties == 2)
  assert(spr.properties.a == a)
  assert(spr.properties.b == b)
end
