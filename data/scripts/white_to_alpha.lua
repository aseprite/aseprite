-- Aseprite
-- Copyright (C) 2015-2018 by David Capello

local pc = app.pixelColor
local img = app.activeImage

for y=0,img.height-1 do
   for x=0,img.width-1 do
    local c = img:getPixel(x, y)
    local r = pc.rgbaR(c)
    local g = pc.rgbaG(c)
    local b = pc.rgbaB(c)
    local a = pc.rgbaA(c)
    if a > 0 then a = 255 - (r+g+b)/3 end
    img:putPixel(x, y, pc.rgba(r, g, b, a))
  end
end
