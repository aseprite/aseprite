-- Copyright (C) 2019-2023  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local file1 = app.params["file1"]
local file2 = app.params["file2"]
if file1 == nil or file2 == nil then
  return 0
end

local data1 = json.decode(io.open(file1):read('a'))
local data2 = json.decode(io.open(file2):read('a'))

if data1 == nil then
  print('Cannot read file ' .. file1)
  return 1
elseif data2 == nil then
  print('Cannot read file ' .. file2)
  return 1
end

local function replace_filename(fn, newfn)
  return string.gsub(fn, "(.*)[/\\]([^/\\]+)", "%1/"..newfn)
end

local sheet1 = app.open(replace_filename(file1, data1.meta.image))
local sheet2 = app.open(replace_filename(file2, data2.meta.image))

for k,v in pairs(data1.frames) do
  local fr1 = data1.frames[k]
  local fr2 = data2.frames[k]
  if fr1.duration ~= fr2.duration then
    print('Frame '..k..' doesn\'t match duration')
    return 1
  end
  if fr1.sourceSize.w ~= fr2.sourceSize.w or
     fr1.sourceSize.h ~= fr2.sourceSize.h then
    print('Frame '..k..' doesn\'t match sourceSize')
    return 1
  end

  local celImage1 = Image(fr1.frame.w, fr1.frame.h, sheet1.colorMode)
  local celImage2 = Image(fr2.frame.w, fr2.frame.h, sheet2.colorMode)
  celImage1:drawSprite(sheet1, 1, Point(-fr1.frame.x, -fr1.frame.y))
  celImage2:drawSprite(sheet2, 1, Point(-fr2.frame.x, -fr2.frame.y))

  local frImage1 = Image(fr1.sourceSize.w, fr1.sourceSize.h, sheet1.colorMode)
  local frImage2 = Image(fr2.sourceSize.w, fr2.sourceSize.h, sheet2.colorMode)
  frImage1:drawImage(celImage1, Point(fr1.spriteSourceSize.x, fr1.spriteSourceSize.y), 255, BlendMode.SRC)
  frImage2:drawImage(celImage2, Point(fr2.spriteSourceSize.x, fr2.spriteSourceSize.y), 255, BlendMode.SRC)

  -- To debug this function
  --frImage1:saveAs(replace_filename(file1, k .. "-fr1.png"))
  --frImage2:saveAs(replace_filename(file2, k .. "-fr2.png"))
  --print(k, "fr1", fr1.frame.x, fr1.frame.y, "fr2", fr2.frame.x, fr2.frame.y)

  assert(frImage1:isEqual(frImage2))
end
