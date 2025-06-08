-- Copyright (C) 2024  Igara Studio S.A.
-- Released under the MIT license. See LICENSE.txt.

dofile('./test_utils.lua')

-- Helper to export sprite to JSON and parse it
local function export_json(sprite, filename)
  -- Create a subfolder for test outputs
  local out_dir = app.fs.joinPath(app.fs.userConfigPath, "userdata_json")
  if not app.fs.isDirectory(out_dir) then
    app.fs.makeDirectory(out_dir)
  end

  local out_json = app.fs.joinPath(out_dir, "_test_userdata_json.json")
  local out_png  = app.fs.joinPath(out_dir, "_test_userdata_json.png")
  local out_ase  = app.fs.joinPath(out_dir, "_test_userdata_json.aseprite")

  print("Exporting to:")
  print("  out_json: " .. out_json)
  print("  out_png:  " .. out_png)
  print("  out_ase:  " .. out_ase)

  sprite:saveAs(out_ase)
  app.command.ExportSpriteSheet{
    ui=false,
    type=SpriteSheetType.PACKED,
    data=SpriteSheetDataFormat.JSON_HASH,
    dataFilename=out_json,
    textureFilename=out_png
  }
  sprite:close()

  print("Trying to open: " .. out_json)
  local f = io.open(out_json, "r")
  if not f then
    print("FAILED to open: " .. out_json)
  end
  local content = f and f:read("*a") or ""
  if f then f:close() end
  return json.decode(content)
end

do
  -- Create a sprite and set various user/extension properties
  local spr = Sprite(16, 16)
  spr.properties.user_defined_data = true
  spr.properties("my_plugin").extension_defined_data = "sample data"

  spr:newLayer()
  spr.layers[2].properties.user_defined_data = true
  spr.layers[2].properties("my_plugin").extension_defined_data = "sample data"

  spr:newTag(0, 0)
  spr.tags[1].properties.user_defined_data = "sample data"
  spr.tags[1].properties("my_plugin").extension_defined_data = 42

  spr:newCel(spr.layers[2], 1)
  spr.layers[2].cels[1].properties.user_defined_data = 42
  spr.layers[2].cels[1].properties("my_plugin").extension_defined_data = "sample data"

  spr:newSlice(0, 0, 8, 8)
  spr.slices[1].properties.user_defined_data = "sample data"
  spr.slices[1].properties("my_plugin").extension_defined_data = false

  local meta = export_json(spr, "_test_userdata_json.aseprite").meta

  -- Sprite properties
  if meta.properties then
    assert(meta.properties.user_defined_data == true)
    assert(meta.properties["my_plugin"].foo == "sample data")
  end

  -- Layer properties
  local layer = meta.layers[2]
  assert(layer ~= nil)
  assert(layer.properties.user_defined_data == true)
  assert(layer.properties["my_plugin"].extension_defined_data == "sample data")

  -- Tag properties
  local tag = meta.frameTags[1]
  assert(tag ~= nil)
  assert(tag.properties.user_defined_data == "sample data")
  assert(tag.properties["my_plugin"].extension_defined_data == 42)

  -- Cel properties
  local cel = meta.layers[2].cels and meta.layers[2].cels[1]
  assert(cel ~= nil)
  assert(cel.properties.user_defined_data == 42)
  assert(cel.properties["my_plugin"].extension_defined_data == "sample data")

  -- Slice properties
  local slice = meta.slices[1]
  assert(slice ~= nil)
  assert(slice.properties.user_defined_data == "sample data")
  assert(slice.properties["my_plugin"].extension_defined_data == false)
end

print("userdata_json.lua: All user data JSON export tests passed!")
