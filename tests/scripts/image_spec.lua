-- Copyright (C) 2018  Igara Studio S.A.
-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  local spec = ImageSpec{colorMode=ColorMode.GRAY, width=32, height=64, transparentColor=2}
  assert(spec.colorMode == ColorMode.GRAY)
  assert(spec.width == 32)
  assert(spec.height == 64)
  assert(spec.transparentColor == 2)
end

do
  local sprite = Sprite(32, 64, ColorMode.INDEXED)
  assert(sprite.width == 32)
  assert(sprite.height == 64)
  assert(sprite.colorMode == ColorMode.INDEXED)

  local sprite2 = Sprite(sprite.spec)
  assert(sprite2.width == 32)
  assert(sprite2.height == 64)
  assert(sprite2.colorMode == ColorMode.INDEXED)

  local spec = sprite.spec
  assert(spec.width == 32)
  assert(spec.height == 64)
  assert(spec.colorMode == ColorMode.INDEXED)

  spec.width = 30
  spec.height = 40
  spec.colorMode = ColorMode.RGB
  assert(spec.width == 30)
  assert(spec.height == 40)
  assert(spec.colorMode == ColorMode.RGB)

  local image = Image(spec)
  assert(image.width == 30)
  assert(image.height == 40)
  assert(image.colorMode == ColorMode.RGB)

  print(image.spec.width, image.spec.height, image.spec.colorMode)
  assert(image.spec.width == 30)
  assert(image.spec.height == 40)
  assert(image.spec.colorMode == ColorMode.RGB)
end
