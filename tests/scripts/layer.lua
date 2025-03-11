-- Copyright (C) 2019  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  local s = Sprite(32, 64)
  local l = s.layers[1]
  assert(l.parent == s)
  assert(l.opacity == 255)
  assert(l.blendMode == BlendMode.NORMAL)

  l.name = "My Layer"
  l.opacity = 128
  l.blendMode = BlendMode.MULTIPLY
  assert(l.name == "My Layer")
  assert(l.opacity == 128)
  assert(l.blendMode == BlendMode.MULTIPLY)

  l.data = "Data"
  assert(l.data == "Data")
end

-- Test layer uuid with uuidsForLayers enabled
do
  local s = Sprite(32, 32)
  local l = s.layers[1]
  assert(s.uuidsForLayers == false)

  local uuid = l.uuid
  assert(uuid ~= nil)

  s.uuidsForLayers = true

  assert(s.uuidsForLayers == true)
  s.filename = "_test_layer_uuid.aseprite"
  app.command.SaveFile()
  app.command.CloseFile()

  app.command.OpenFile { filename = "_test_layer_uuid.aseprite" }
  assert(app.sprite.uuidsForLayers == true)
  assert(app.sprite.layers[1].uuid == uuid)
end

-- Test layer uuid with uuidsForLayers disabled
do
  local s = Sprite(32, 32)
  local l = s.layers[1]
  assert(s.uuidsForLayers == false)

  local uuid = l.uuid
  assert(uuid ~= nil)

  s.filename = "_test_layer_uuid_2.aseprite"
  app.command.SaveFile()
  app.command.CloseFile()

  app.command.OpenFile { filename = "_test_layer_uuid_2.aseprite" }
  assert(app.sprite.uuidsForLayers == false)
  -- UUIDs are not equal because it was not saved due to uuidsForLayers being
  -- disabled at the time of saving the file.
  assert(app.sprite.layers[1].uuid ~= uuid)
end
