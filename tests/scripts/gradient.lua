-- Copyright (C) 2026-present  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

dofile('./test_utils.lua')

local spr = Sprite(4, 4, ColorMode.GRAYSCALE)

app.useTool {
  tool = 'rectangular_marquee',
  points = { Point(2, 0), Point(3, 3) }}

app.useTool {
  tool = 'rectangular_marquee',
  selection = SelectionMode.ADD,
  points = { Point(0, 2), Point(1, 3) }}

local gray = app.pixelColor.graya
local c0 = gray(0)
local c1 = gray(85)
local c2 = gray(170)
local c3 = gray(255)

-- Horizontal Gradient
app.useTool {
  tool = 'gradient',
  color = c0,
  bgColor = c3,
  contiguous = false,
  points = { Point(3, 3), Point(0, 3) }}

local img = app.cel.image
print(img.width)
print(img.height)
expect_img(img, {  0,  0, c1, c0,
                   0,  0, c1, c0,
                  c3, c2, c1, c0,
                  c3, c2, c1, c0 })

app.undo()

-- Vertical Gradient
app.useTool {
  tool = 'gradient',
  color = c0,
  bgColor = c3,
  contiguous = false,
  points = { Point(3, 3), Point(3, 0) }}

  expect_img(img, {  0,  0, c3, c3,
                     0,  0, c2, c2,
                    c1, c1, c1, c1,
                    c0, c0, c0, c0 })
