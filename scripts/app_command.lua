-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local s = Sprite(32, 32)
assert(s.width == 32)
assert(s.height == 32)

s:resize(50, 40)
assert(s.width == 50)
assert(s.height == 40)

-- Undo/Redo

local pc = app.command.Undo()
assert(s.width == 32)
assert(s.height == 32)

local pc = app.command.Redo()
assert(s.width == 50)
assert(s.height == 40)

-- NewLayer

assert(#s.layers == 1)
app.command.NewLayer{top=true}
assert(#s.layers == 2)
assert(s.layers[2].isImage)

app.command.NewLayer{top=true, group=true}
assert(#s.layers == 3)
assert(s.layers[3].isGroup)
