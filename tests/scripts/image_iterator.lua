-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local pc = app.pixelColor

-- Iterate pixels
do
   local spr = Sprite(2, 2)
   local image = app.site.image
   local colors = { pc.rgba(255, 0, 0, 255),
                    pc.rgba(0, 255, 0, 255),
                    pc.rgba(255, 0, 255, 255),
                    pc.rgba(255, 0, 255, 255) }
   local xy = {
      { x=0, y=0 },
      { x=1, y=0 },
      { x=0, y=1 },
      { x=1, y=1 } }

   local c = 1
   for y=0,image.height-1 do
      for x=0,image.width-1 do
         image:putPixel(x, y, colors[c])
         c = c+1
      end
   end

   c = 1
   for y=0,image.height-1 do
      for x=0,image.width-1 do
         assert(colors[c] == image:getPixel(x, y))
         c = c+1
      end
   end

   c = 1
   for it in image:pixels() do
      assert(colors[c] == it())
      assert(xy[c].x == it.x)
      assert(xy[c].y == it.y)
      c = c+1
   end

   c = 0
   for it in image:pixels{x=1, y=0, width=1, height=2} do
     local i = 1 + it.y*2 + it.x
     assert(colors[i] == it())
     assert(xy[i].x == it.x)
     assert(xy[i].y == it.y)
     c = c + 1
   end
   assert(c == 2)

   -- Iterating outside
   for it in image:pixels{x=2, y=0, width=2, height=2} do
     assert(false)
   end

   c = 1
   for it in image:pixels() do
      it(pc.rgba(255, 32*c, 0, 255))
      c = c+1
   end

   c = 1
   for it in image:pixels() do
      assert(pc.rgba(255, 32*c, 0, 255) == it())
      c = c+1
   end
end
