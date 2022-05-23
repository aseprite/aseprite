-- Copyright (C) 2022  Igara Studio S.A.
-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local a, b

a = Color()
assert(a.red == 0)
assert(a.green == 0)
assert(a.blue == 0)
assert(a.alpha == 0)

a = Color{ r=100, g=50, b=10 }
b = Color(100, 50, 10)
assert(a.red == 100)
assert(a.green == 50)
assert(a.blue == 10)
assert(a.alpha == 255)
assert(a == b)

a = Color{ red=200, green=100, blue=20, alpha=200 }
b = Color(200, 100, 20, 200)
assert(a.red == 200)
assert(a.green == 100)
assert(a.blue == 20)
assert(a.alpha == 200)
assert(a == b)
print(a.rgbaPixel)
print(app.pixelColor.rgba(200, 100, 20, 200))
assert(a.rgbaPixel == app.pixelColor.rgba(200, 100, 20, 200))

a = Color{ h=180, s=0.4, v=0.5, a=200 }
b = Color{ hue=180, saturation=0.4, value=0.5, alpha=200 }
assert(a.hue == 180)
assert(a.saturation == 0.4)
assert(a.value == 0.5)
assert(a.alpha == 200)
assert(b.hue == 180)
assert(b.saturation == 0.4)
assert(b.value == 0.5)
assert(b.alpha == 200)
assert(a == b)
assert(a ~= 1) -- Comparing with other type fails
