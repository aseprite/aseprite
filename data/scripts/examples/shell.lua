-- ASE - Allegro Sprite Editor
-- Copyright (C) 2001-2005, 2007 by David A. Capello

ResetConfig()

sprite = NewSprite(IMAGE_RGB, 128, 96)
undo_disable(sprite.undo)
ClearMask()

local points = ""
local x, y, angle

SetBrush("circle 2 0")
SetDrawMode("glass 32")

for angle = 0, 360 do
  x = 48 + cos(PI*angle/180) * (64*angle/360)
  y = 64 + sin(PI*angle/180) * (64*angle/360)
  points = points .. " " .. x .. "," .. y

  ToolTrace("line 48,64 "..x..","..y)
end

ConvolutionMatrixRGBA("smooth-3x3")
ColorCurveRGBA({ 0,0, 132,41, 187,131, 211,251, 255,255 })

undo_enable(sprite.undo)
sprite_show(sprite)

RestoreConfig()
