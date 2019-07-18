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

	local a = value
	local b = expected
	print(debug.traceback())
	print('Expected A == B but:')
	if image.colorMode == ColorMode.RGB then
	  print(string.format(' - Value A = rgba(%d,%d,%d,%d)',
			      app.pixelColor.rgbaR(a),
			      app.pixelColor.rgbaG(a),
			      app.pixelColor.rgbaB(a),
			      app.pixelColor.rgbaA(a)))
	  print(string.format(' - Value B = rgba(%d,%d,%d,%d)',
			      app.pixelColor.rgbaR(b),
			      app.pixelColor.rgbaG(b),
			      app.pixelColor.rgbaB(b),
			      app.pixelColor.rgbaA(b)))
	elseif image.ColorMode == ColorMode.GRAY then
	  print(string.format(' - Value A = gray(%d,%d)',
			      app.pixelColor.grayG(a),
			      app.pixelColor.grayA(a)))
	  print(string.format(' - Value B = gray(%d,%d)',
			      app.pixelColor.grayV(b),
			      app.pixelColor.grayA(b)))
	else
	  print(' - Value A = ' .. tostring(a))
	  print(' - Value B = ' .. tostring(b))
	end
	assert(a == b)
      end
    end
  end
end
