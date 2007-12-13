-- ASE - Allegro Sprite Editor
-- Copyright (C) 2001-2005, 2007 by David A. Capello

local x, y, points

NewSprite(IMAGE_RGB, 128, 128)
ClearMask()

points = ""
for y = 0, 128 do
  x = 64 + cos(PI*y/128) * 63
  points = points .. " " .. x .. "," .. y
end
ToolTrace("pencil " .. points)
ToolTrace("floodfill 127,127")

ConvolutionMatrixRGBA("smooth-5x5")
ColorCurveRGBA({ 0,0, 128-32,16, 128+32,256-16, 255,255 })
