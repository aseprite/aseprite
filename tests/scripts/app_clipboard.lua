-- Copyright (C) 2024  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

dofile('./test_utils.lua')

do -- Clipboard clearing
  app.clipboard.text = "hello world"
  expect_eq(false, app.clipboard.isEmpty)
  app.clipboard.clear()
  expect_eq(true, app.clipboard.isEmpty)
end

do -- Text copying and access
  app.clipboard.clear()

  expect_eq(false, app.clipboard.isText)
  expect_eq(false, app.clipboard.isImage)
  expect_eq(true, app.clipboard.isEmpty)

  app.clipboard.text = "hello world"

  expect_eq(true, app.clipboard.isText)
  expect_eq(false, app.clipboard.isImage)
  expect_eq(false, app.clipboard.isEmpty)

  expect_eq("hello world", app.clipboard.text)
end

do -- Image copying and access
  local sprite = Sprite{ fromFile="sprites/abcd.aseprite" }

  app.clipboard.clear()

  expect_eq(false, app.clipboard.isText)
  expect_eq(false, app.clipboard.isImage)
  expect_eq(true, app.clipboard.isEmpty)

  assert(app.image ~= nil)
  app.clipboard.image = app.image

  expect_eq(false, app.clipboard.isText)
  expect_eq(true, app.clipboard.isImage)
  expect_eq(false, app.clipboard.isEmpty)

  expect_eq(app.image.width, app.clipboard.image.width)
  expect_eq(app.image.height, app.clipboard.image.height)
  expect_eq(app.image.bytes, app.clipboard.image.bytes)
end
