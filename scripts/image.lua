-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local pc = app.pixelColor

local a = Image(32, 64)
assert(a.width == 32)
assert(a.height == 64)
assert(a.colorMode == ColorMode.RGB) -- RGB by default

do
   local b = Image(32, 64, ColorMode.INDEXED)
   assert(b.width == 32)
   assert(b.height == 64)
   assert(b.colorMode == ColorMode.INDEXED)
end

-- Get/put RGBA pixels
do
   for y=0,a.height-1 do
      for x=0,a.width-1 do
         a:putPixel(x, y, pc.rgba(x, y, x+y, x-y))
      end
   end
end

-- Clone
do
   local c = a:clone()
   assert(c.width == 32)
   assert(c.height == 64)
   assert(c.colorMode == ColorMode.RGB)

   -- Get RGB pixels
   for y=0,c.height-1 do
      for x=0,c.width-1 do
         assert(c:getPixel(x, y) == pc.rgba(x, y, x+y, x-y))
      end
   end
end

-- Patch
do
   local spr = Sprite(256, 256)
   local image = app.site.image
   local copy = image:clone()
   assert(image:getPixel(0, 0) == 0)
   for y=0,copy.height-1 do
      for x=0,copy.width-1 do
         copy:putPixel(x, y, pc.rgba(255-x, 255-y, 0, 255))
      end
   end
   image:putImage(copy)
   assert(image:getPixel(0, 0) == pc.rgba(255, 255, 0, 255))
   assert(image:getPixel(255, 255) == pc.rgba(0, 0, 0, 255))
   app.undo()
   assert(image:getPixel(0, 0) == pc.rgba(0, 0, 0, 0))
   assert(image:getPixel(255, 255) == pc.rgba(0, 0, 0, 0))
end
