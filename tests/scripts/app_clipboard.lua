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
  local sprite = Sprite{ fromFile="sprites/abcd.aseprite" }

  app.clipboard.clear()

  expect_eq(false, app.clipboard.hasText)
  expect_eq(false, app.clipboard.hasImage)
  assert(app.image ~= nil)

  app.clipboard.content = {
    image = app.image,
    palettte = sprite.palettes[1],
    mask = sprite.spec.transparentColor,
    tileset = nil,
    text = nil, -- TODO: Error when this happens
  }

  expect_eq(false, app.clipboard.hasText)
  expect_eq(true, app.clipboard.hasImage)

  local result = app.clipboard.content
  assert(result ~= nil)

  expect_eq(sprite.image.bytes, c.image.bytes)
  -- TODO: the rest
end
