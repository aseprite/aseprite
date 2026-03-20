-- Copyright (C) 2026-present  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  local s = Sprite{ fromFile = "sprites/z-order.aseprite" }
  local L1 = s.layers[1]
  local L2 = s.layers[2]
  local L3 = s.layers[3]
  local L4 = s.layers[4]
  local G1 = s.layers[5]
  local L5 = s.layers[6]

  app.preferences.experimental.compose_groups = true
  L4.cels[1].zIndex = -1
  G1.isVisible = false
  L5.isVisible = false

  app.useTool { tool = 'eyedropper',
                points = { Point(1,5) } }
  assert(app.fgColor.rgbaPixel == app.pixelColor.rgba(0, 255, 0))
  L3.isVisible = false
  app.useTool { tool = 'eyedropper',
                points = { Point(1,5) } }
  assert(app.fgColor.rgbaPixel == app.pixelColor.rgba(255, 0, 0))
  L3.isVisible = true

  L4.cels[1].zIndex = 1
  G1.isVisible = true
  L5.isVisible = true
  app.useTool { tool = 'eyedropper',
                points = { Point(0,0) } }
  assert(app.fgColor.rgbaPixel == app.pixelColor.rgba(64, 64, 64))
  G1.isVisible = false
  app.useTool { tool = 'eyedropper',
                points = { Point(0,0) } }
  assert(app.fgColor.rgbaPixel == app.pixelColor.rgba(64, 64, 64))
end
