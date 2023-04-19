-- Copyright (C) 2018-2023  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local s = Sprite(32, 64)
assert(s == app.sprite)
assert(s == app.activeSprite)
assert(s == app.site.sprite)
assert(s == app.frame.sprite)
assert(s == app.activeFrame.sprite)
assert(s == app.site.frame.sprite)
assert(1 == app.activeFrame.frameNumber)
assert(1 == app.site.frame.frameNumber)
assert(1 == app.site.frameNumber)
assert(0.100 == app.activeFrame.duration) -- Default frame duration
assert(0.100 == app.site.frame.duration)
assert(s == app.layer.sprite)
assert(s == app.activeLayer.sprite)
assert(s == app.site.layer.sprite)
assert(s == app.cel.sprite)
assert(s == app.activeCel.sprite)
assert(s == app.site.cel.sprite)

app.frame.duration = 0.8

app.command.NewFrame()
assert(2 == app.frame.frameNumber)
assert(0.8 == app.frame.duration) -- Copy frame duration of previous frame
