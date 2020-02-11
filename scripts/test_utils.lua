-- Copyright (C) 2019-2020  Igara Studio S.A.
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

local function dump_img(image)
  local w = image.width
  local h = image.height
  print('Image(' .. tostring(w) .. 'x' .. tostring(h) .. ') = {')
  for v=0,h-1 do
    local lineStr = '  '
    for u=0,w-1 do
      lineStr = lineStr .. image:getPixel(u, v) .. ','
    end
    print(lineStr)
  end
  print('}')
end

function expect_img(image, expectedPixels)
  local w = image.width
  local h = image.height
  if w*h ~= #expectedPixels then
    print(debug.traceback())
    dump_img(image)
    assert(w*h == #expectedPixels)
  end
  for y=0,h-1 do
    for x=0,w-1 do
      local value = image:getPixel(x, y)
      local expected = expectedPixels[1+y*w+x]
      if value ~= expected then
	dump_img(image)
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
	elseif image.colorMode == ColorMode.GRAY then
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

function array_to_pixels(array, image)
  local w = image.width
  local h = image.height
  assert(w*h == #array)
  local i = 1
  for y=0,h-1 do
    for x=0,w-1 do
      image:drawPixel(x, y, array[i])
      i = i+1
    end
  end
end

-- Returns true if the given array (layers) of layers
function expect_rendered_layers(expectedImage, sprite, layerNames, frame)
  function contains_layer(name)
    for _,n in ipairs(layerNames) do
      if name == n then return true end
    end
    return false
  end

  if frame == nil then frame = 1 end
  local render = Image(sprite.spec)
  function render_layers(prefix, layers)
    for _,layer in ipairs(layers) do
      if layer.isGroup then
	render_layers(layer.name.."/", layer.layers)
      end
      if contains_layer(prefix..layer.name) then
	local cel = layer:cel(frame)
	if cel then
	  render:drawImage(cel.image, cel.position)
	end
      end
    end
  end

  render_layers("", sprite.layers)
  if not expectedImage:isEqual(render) then
    print("Rendering:")
    for _,n in ipairs(layerNames) do
      print("  - " .. n)
    end
    error("render doesn't match to the expected image")
  end
end
