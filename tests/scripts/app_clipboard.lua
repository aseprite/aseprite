-- Copyright (C) 2024  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

dofile('./test_utils.lua')

do -- Text clearing
  app.clipboard.text = "clear me"
  expect_eq(true, app.clipboard.hasText)
  app.clipboard.clear()
  expect_eq(false, app.clipboard.hasText)
end

do -- Text clearing (with content)
  app.clipboard.content = { text = "clear me 2" }
  expect_eq(true, app.clipboard.hasText)
  app.clipboard.clear()
  expect_eq(false, app.clipboard.hasText)
end

do -- Text copying and access
  app.clipboard.clear()

  expect_eq(false, app.clipboard.hasText)
  expect_eq(false, app.clipboard.hasImage)

  app.clipboard.text = "hello world"

  expect_eq(true, app.clipboard.hasText)
  expect_eq(false, app.clipboard.hasImage)

  expect_eq("hello world", app.clipboard.text)
end

do -- Text copying and access (with .content)
  app.clipboard.clear()

  expect_eq(false, app.clipboard.hasText)
  expect_eq(false, app.clipboard.hasImage)

  app.clipboard.content = { text = "hello world 2"}

  expect_eq(true, app.clipboard.hasText)
  expect_eq(false, app.clipboard.hasImage)

  expect_eq("hello world 2", app.clipboard.content.text)
end

do -- Image copying and access
  local sprite = Sprite{ fromFile="sprites/abcd.aseprite" }

  app.clipboard.clear()

  expect_eq(false, app.clipboard.hasText)
  expect_eq(false, app.clipboard.hasImage)

  assert(app.image ~= nil)
  app.clipboard.image = app.image

  expect_eq(false, app.clipboard.hasText)
  expect_eq(true, app.clipboard.hasImage)

  expect_eq(app.image.width, app.clipboard.image.width)
  expect_eq(app.image.height, app.clipboard.image.height)
  expect_eq(app.image.bytes, app.clipboard.image.bytes)
end

do -- Image copying and access (with .content)
  -- TODO: Using the previous image for now to avoid the IMAGE_TILEMAP format not being supported.
  local beforeSprite = Sprite{ fromFile="sprites/abcd.aseprite" }
  local imageBefore = app.image:clone()

  local sprite = Sprite{ fromFile="sprites/2x2tilemap2x2tile.aseprite" }
  assert(app.image ~= nil)

  if imageBefore ~= nil then
    expect_eq(false, imageBefore:isEqual(app.image))
  end

  app.clipboard.clear()

  expect_eq(false, app.clipboard.hasText)
  expect_eq(false, app.clipboard.hasImage)
  assert(app.image ~= nil)

  app.clipboard.content = {
    image = imageBefore,
    palettte = sprite.palettes[1],
    mask = sprite.selection,
    tileset = app.layer.tileset
  }

  expect_eq(false, app.clipboard.hasText)
  expect_eq(true, app.clipboard.hasImage)

  local c = app.clipboard.content
  assert(c ~= nil)

  assert(imageBefore:isEqual(c.image))
  expect_eq(sprite.palettes[1]:getColor(1).rgbaPixel, c.palette:getColor(1).rgbaPixel)
  assert(app.layer.tileset:tile(0).image:isEqual(c.tileset:tile(0).image))
  expect_eq(sprite.selection, c.mask)
end
