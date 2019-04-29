-- Copyright (C) 2019  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

function expect_eq(a, b)
  if a ~= b then
    print(debug.traceback())
    print('Expected A == B but:')
    print(' - Value A = ' .. tostring(a))
    print(' - Value B = ' .. tostring(b))
    assert(a == b)
  end
end

function expect_img(image, expectedPixels)
  local w = image.width
  local h = image.height
  for y=0,h-1 do
    for x=0,w-1 do
      local value = image:getPixel(x, y)
      local expected = expectedPixels[1+y*w+x]
      if value ~= expected then
        print('Image(' .. tostring(w) .. 'x' .. tostring(h) .. ') = {')
        for v=0,h-1 do
          lineStr = '  '
          for u=0,w-1 do
            lineStr = lineStr .. image:getPixel(u, v) .. ','
          end
          print(lineStr)
        end
        print('}')
        print('In pixel (' .. x .. ', ' .. y .. '):')
        expect_eq(value, expected)
      end
    end
  end
end
