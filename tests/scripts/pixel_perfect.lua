-- Copyright (C) 2019-2021  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

dofile('./test_utils.lua')

local spr = Sprite(6, 6)
local cel = spr.cels[1]

-- Point size 1px, solid color, no symmetry, no tiled mode
do
  local title = '1px, solid, no symmetry, no tiled'
  local red = Color{ r=255, g=0, b=0 }
  local r = red.rgbaPixel
  local pixel=Brush{ size=1, type=BrushType.CIRCLE }
  local testData = {
    {
      id='1 - ' .. title .. ': right then down',
      points={ Point(2, 2), Point(3, 2), Point(3, 3) },
      expected={ r, 0,
                 0, r }
    },
    {
      id='2 - ' .. title .. ': down then right',
      points={ Point(2, 2), Point(2, 3), Point(3, 3) },
      expected={ r, 0,
                 0, r }
    },
    {
      id='3 - ' .. title .. ': left then up',
      points={ Point(2, 2), Point(1, 2), Point(1, 1) },
      expected={ r, 0,
                 0, r }
    },
    {
      id='4 - ' .. title .. ': up then left',
      points={ Point(2, 2), Point(2, 1), Point(1, 1) },
      expected={ r, 0,
                 0, r }
    }
  }

  for i,v in ipairs(testData) do
    app.useTool{
      tool='pencil',
      freehandAlgorithm=1,
      brush=pixel,
      color=red,
      points=v.points}
    expect_img_msg(cel.image, v.expected, '\nTest \'' .. v.id .. '\' failed')
    cel.image:clear(0)
  end
end

-- Point size 2px, translucent color, no symmetry, no tiled mode
do
  local title = '2px, translucent, no symmetry, no tiled'
  local red = Color{ r=255, g=0, b=0, a=127 }
  local r = red.rgbaPixel
  local square=Brush{ size=2, type=BrushType.SQUARE }
  local testData = {
    {
      id='1 - ' .. title .. ': right then down',
      points={ Point(2, 2), Point(3, 2), Point(3, 3) },
      expected={ r, r, 0,
                 r, r, r,
                 0, r, r }
    },
    {
      id='2 - ' .. title .. ': down then right',
      points={ Point(2, 2), Point(2, 3), Point(3, 3) },
      expected={ r, r, 0,
                 r, r, r,
                 0, r, r }
    },
    {
      id='3 - ' .. title .. ': left then up',
      points={ Point(2, 2), Point(1, 2), Point(1, 1) },
      expected={ r, r, 0,
                 r, r, r,
                 0, r, r }
    },
    {
      id='4 - ' .. title .. ': up then left',
      points={ Point(2, 2), Point(2, 1), Point(1, 1) },
      expected={ r, r, 0,
                 r, r, r,
                 0, r, r }
    }
  }

  for i,v in ipairs(testData) do
    app.useTool{
      tool='pencil',
      freehandAlgorithm=1,
      brush=square,
      color=red,
      points=v.points}
    expect_img_msg(cel.image, v.expected, '\nTest \'' .. v.id .. '\' failed')
    cel.image:clear(0)
  end
end

-- Point size 2px, translucent color, symmetry, no tiled mode
do
  local pref = app.preferences
  local docPref = pref.document(spr)
  pref.symmetry_mode.enabled = true
  docPref.symmetry.mode   = 3
  docPref.symmetry.x_axis = 3
  docPref.symmetry.y_axis = 3

  local title = '2px, translucent, symmetry on, no tiled'
  local red = Color{ r=255, g=0, b=0, a=127 }
  local r = red.rgbaPixel
  local square=Brush{ size=2, type=BrushType.SQUARE }
  local testData = {
    {
      id='1 - ' .. title .. ': right then down',
      points={ Point(1, 1), Point(2, 1), Point(2, 2) },
      expected={ r, r, 0, 0, r, r,
                 r, r, r, r, r, r,
                 0, r, r, r, r, 0,
                 0, r, r, r, r, 0,
                 r, r, r, r, r, r,
                 r, r, 0, 0, r, r }
    },
    {
      id='2 - ' .. title .. ': down then right',
      points={ Point(1, 1), Point(1, 2), Point(2, 2) },
      expected={ r, r, 0, 0, r, r,
                 r, r, r, r, r, r,
                 0, r, r, r, r, 0,
                 0, r, r, r, r, 0,
                 r, r, r, r, r, r,
                 r, r, 0, 0, r, r }
    },
    {
      id='3 - ' .. title .. ': left then up',
      points={ Point(2, 2), Point(1, 2), Point(1, 1) },
      expected={ r, r, 0, 0, r, r,
                 r, r, r, r, r, r,
                 0, r, r, r, r, 0,
                 0, r, r, r, r, 0,
                 r, r, r, r, r, r,
                 r, r, 0, 0, r, r }
    },
    {
      id='4 - ' .. title .. ': up then left',
      points={ Point(2, 2), Point(2, 1), Point(1, 1) },
      expected={ r, r, 0, 0, r, r,
                 r, r, r, r, r, r,
                 0, r, r, r, r, 0,
                 0, r, r, r, r, 0,
                 r, r, r, r, r, r,
                 r, r, 0, 0, r, r }
    }
  }

  for i,v in ipairs(testData) do
    app.useTool{
      tool='pencil',
      freehandAlgorithm=1,
      brush=square,
      color=red,
      points=v.points}
    expect_img_msg(cel.image, v.expected, '\nTest \'' .. v.id .. '\' failed')
    cel.image:clear(0)
  end
end

-- Point size 2px, translucent color, no symmetry, tiled mode on
do
  local pref = app.preferences
  local docPref = pref.document(spr)
  pref.symmetry_mode.enabled = false
  docPref.tiled.mode = 3

  local title = '2px, translucent, no symmetry, tiled'
  local red = Color{ r=255, g=0, b=0, a=127 }
  local r = red.rgbaPixel
  local square=Brush{ size=2, type=BrushType.SQUARE }
  local testData = {
    -- Top left corner
    {
      id='1 - ' .. title .. ': on top left corner, right then down',
      points={ Point(0, 0), Point(1, 0), Point(1, 1) },
      expected={ r, r, 0, 0, 0, r,
                 r, r, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 r, 0, 0, 0, 0, r }
    },
    {
      id='2 - ' .. title .. ': on top left corner, down then right',
      points={ Point(0, 0), Point(0, 1), Point(1, 1) },
      expected={ r, r, 0, 0, 0, r,
                 r, r, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 r, 0, 0, 0, 0, r }
    },
    {
      id='3 - ' .. title .. ': on top left corner, left then up',
      points={ Point(0, 0), Point(-1, 0), Point(-1, -1) },
      expected={ r, 0, 0, 0, 0, r,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, r, r,
                 r, 0, 0, 0, r, r }
    },
    {
      id='4 - ' .. title .. ': on top left corner, up then left',
      points={ Point(0, 0), Point(0, -1), Point(-1, -1) },
      expected={ r, 0, 0, 0, 0, r,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, r, r,
                 r, 0, 0, 0, r, r }
    },
    -- Top right corner
    {
      id='5 - ' .. title .. ': on top right corner, right then down',
      points={ Point(6, 0), Point(7, 0), Point(7, 1) },
      expected={ r, r, 0, 0, 0, r,
                 r, r, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 r, 0, 0, 0, 0, r }
    },
    {
      id='6 - ' .. title .. ': on top right corner, down then right',
      points={ Point(6, 0), Point(6, 1), Point(7, 1) },
      expected={ r, r, 0, 0, 0, r,
                 r, r, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 r, 0, 0, 0, 0, r }
    },
    {
      id='7 - ' .. title .. ': on top right corner, left then up',
      points={ Point(6, 0), Point(5, 0), Point(5, -1) },
      expected={ r, 0, 0, 0, 0, r,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, r, r,
                 r, 0, 0, 0, r, r }
    },
    {
      id='8 - ' .. title .. ': on top right corner, up then left',
      points={ Point(6, 0), Point(5, 0), Point(5, -1) },
      expected={ r, 0, 0, 0, 0, r,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, r, r,
                 r, 0, 0, 0, r, r }
    },
    -- Bottom left corner
    {
      id='9 - ' .. title .. ': on bottom left corner, right then down',
      points={ Point(0, 6), Point(1, 6), Point(1, 7) },
      expected={ r, r, 0, 0, 0, r,
                 r, r, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 r, 0, 0, 0, 0, r }
    },
    {
      id='10 - ' .. title .. ': on bottom left corner, down then right',
      points={ Point(0, 6), Point(0, 7), Point(1, 7) },
      expected={ r, r, 0, 0, 0, r,
                 r, r, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 r, 0, 0, 0, 0, r }
    },
    {
      id='11 - ' .. title .. ': on bottom left corner, left then up',
      points={ Point(0, 6), Point(-1, 6), Point(-1, 5) },
      expected={ r, 0, 0, 0, 0, r,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, r, r,
                 r, 0, 0, 0, r, r }
    },
    {
      id='12 - ' .. title .. ': on bottom left corner, up then left',
      points={ Point(0, 6), Point(0, 5), Point(-1, 5) },
      expected={ r, 0, 0, 0, 0, r,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, r, r,
                 r, 0, 0, 0, r, r }
    },
    -- Botomm right corner
    {
      id='13 - ' .. title .. ': on bottom right corner, right then down',
      points={ Point(6, 6), Point(7, 6), Point(7, 7) },
      expected={ r, r, 0, 0, 0, r,
                 r, r, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 r, 0, 0, 0, 0, r }
    },
    {
      id='14 - ' .. title .. ': on bottom right corner, down then right',
      points={ Point(6, 6), Point(6, 7), Point(7, 7) },
      expected={ r, r, 0, 0, 0, r,
                 r, r, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 r, 0, 0, 0, 0, r }
    },
    {
      id='15 - ' .. title .. ': on bottom right corner, left then up',
      points={ Point(6, 6), Point(5, 6), Point(5, 5) },
      expected={ r, 0, 0, 0, 0, r,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, r, r,
                 r, 0, 0, 0, r, r }
    },
    {
      id='16 - ' .. title .. ': on bottom right corner, up then left',
      points={ Point(6, 6), Point(6, 5), Point(5, 5) },
      expected={ r, 0, 0, 0, 0, r,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, r, r,
                 r, 0, 0, 0, r, r }
    },
  }

  for i,v in ipairs(testData) do
    app.useTool{
      tool='pencil',
      freehandAlgorithm=1,
      brush=square,
      color=red,
      points=v.points}
    expect_img_msg(cel.image, v.expected, '\nTest \'' .. v.id .. '\' failed')
    cel.image:clear(0)
  end
end
