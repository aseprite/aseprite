-- Copyright (C) 2018  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local s = Sprite(32, 64)
assert(s == app.activeSprite)
assert(s == app.site.sprite)
assert(s == app.activeCel.sprite)
assert(s == app.site.cel.sprite)
assert(s == app.activeFrame.sprite)
assert(s == app.site.frame.sprite)
assert(1 == app.activeFrame.frameNumber)
assert(1 == app.site.frame.frameNumber)
assert(0.100 == app.activeFrame.duration) -- Default frame duration
assert(0.100 == app.site.frame.duration)

-- TODO fix these tests when there is no UI
--app.command.NewFrame()
--assert(2 == app.activeFrame.frameNumber)
--assert(0.100 == app.activeFrame.duration) -- Default frame duration
