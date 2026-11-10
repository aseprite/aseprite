-- Copyright (C) 2026  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local s = Sprite(16, 16)
app.sprite = s

local l1 = s.layers[1]
local l2 = s:newLayer()
local g = s:newGroup()
local gl = s:newLayer()

l1.name = "A"
l2.name = "B"
g.name = "G"
gl.name = "C"

gl.parent = g

l1.isVisible = true
l2.isVisible = false
g.isVisible = false
gl.isVisible = true

app.command.ToggleAllLayers()

assert(not l1.isVisible)
assert(not l2.isVisible)
assert(not g.isVisible)
assert(not gl.isVisible)

app.undo()

assert(l1.isVisible)
assert(not l2.isVisible)
assert(not g.isVisible)
assert(gl.isVisible)

app.redo()

assert(not l1.isVisible)
assert(not l2.isVisible)
assert(not g.isVisible)
assert(not gl.isVisible)
