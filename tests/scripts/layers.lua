-- Copyright (C) 2019-2022  Igara Studio S.A.
-- Copyright (C) 2018  David Capello
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  local s = Sprite(32, 32)
  assert(#s.layers == 1)

  local a = s.layers[1]
  local b = s:newLayer()
  local c = s:newLayer()

  assert(#s.layers == 3)
  assert(s.layers[1] == a)
  assert(s.layers[2] == b)
  assert(s.layers[3] == c)

  local i = 1
  for k,v in ipairs(s.layers) do
    assert(i == k)
    assert(v == s.layers[k])
    i = i+1
  end

  s:deleteLayer(b)
  assert(#s.layers == 2)
  assert(s.layers[1] == a)
  assert(s.layers[2] == c)
end

-- Test groups

do
  local s = Sprite(32, 64)
  local l = s.layers[1]
  assert(#s.layers == 1)
  local g = s:newGroup()
  assert(#s.layers == 2)
  assert(l.parent == s)
  assert(g.parent == s)
  assert(s.layers[1] == l)
  assert(s.layers[2] == g)

  l.parent = g
  assert(g.parent == s)
  assert(l.parent == g)

  assert(#s.layers == 1)
  assert(s.layers[1] == g)
  assert(#s.layers[1].layers == 1)
  assert(s.layers[1].layers[1] == l)
end

do
  local s = Sprite(4, 4)
  -- 4 layers
  local a = s.layers[1]    a.name = "a"
  local b = s:newLayer()   b.name = "b"
  local c = s:newLayer()   c.name = "c"
  local d = s:newLayer()   d.name = "d"
  -- 3 groups
  local e = s:newGroup()   e.name = "e"
  local f = s:newGroup()   f.name = "f"
  local g = s:newGroup()   g.name = "g"

  e.parent = f
  a.parent = f
  b.parent = f
  c.parent = g
  d.parent = g

  assert(#s.layers == 2)
  assert(s.layers[1] == f)
  assert(s.layers[2] == g)
  assert(s.layers["f"] == f)
  assert(s.layers["g"] == g)
  assert(f.stackIndex == 1)
  assert(g.stackIndex == 2)

  assert(#f.layers == 3)
  assert(f.layers[1] == e)
  assert(f.layers[2] == a)
  assert(f.layers[3] == b)
  assert(f.layers["e"] == e)
  assert(f.layers["a"] == a)
  assert(f.layers["b"] == b)
  assert(e.stackIndex == 1)
  assert(a.stackIndex == 2)
  assert(b.stackIndex == 3)

  assert(#g.layers == 2)
  assert(g.layers[1] == c)
  assert(g.layers[2] == d)
  assert(g.layers["c"] == c)
  assert(g.layers["d"] == d)
  assert(c.stackIndex == 1)
  assert(d.stackIndex == 2)

  d.stackIndex = 1
  assert(d.stackIndex == 1)
  assert(c.stackIndex == 2)

  d.stackIndex = 2
  assert(c.stackIndex == 1)
  assert(d.stackIndex == 2)

  c.stackIndex = 2
  assert(d.stackIndex == 1)
  assert(c.stackIndex == 2)
end

-- Test possible bugs with stackIndex
do
  local s = Sprite(4, 4)
  local a = s.layers[1]    a.name = "a"
  local b = s:newLayer()   b.name = "b"
  local c = s:newLayer()   c.name = "c"
  local d = s:newLayer()   d.name = "d"
  assert(d.stackIndex == 4)
  assert(s.layers[4].name == "d")

  d.stackIndex = d.stackIndex+1
  assert(d.stackIndex == 4)
  assert(s.layers[4].name == "d")

  -- Go down in the stack
  d.stackIndex = d.stackIndex-1
  assert(s.layers[3].name == "d")

  d.stackIndex = d.stackIndex-1
  assert(s.layers[2].name == "d")

  -- Without change
  d.stackIndex = d.stackIndex
  assert(s.layers[2].name == "d")

  -- Go up
  d.stackIndex = d.stackIndex+1
  assert(d.stackIndex == 3)
  assert(s.layers[3].name == "d")

  d.stackIndex = d.stackIndex+1
  assert(d.stackIndex == 4)
  assert(s.layers[4].name == "d")

  -- Go specific stack indexes
  for i=1,4 do
    d.stackIndex = i
    assert(d.stackIndex == i)
    assert(s.layers[i].name == "d")
  end
end

-- Test crash because ActiveSiteHandler::onBeforeRemoveLayer() didn't
-- update the selected range of layers correctly (i.e. removing
-- deleted layer from the selected layers in the active range).
do
  local s = Sprite(4, 4)
  local a  = s:newGroup()   a.name = "a"
  local b  = s:newGroup()   b.name = "b"
  local c  = s:newGroup()   c.name = "c"
  local ca = s:newGroup()  ca.name = "ca"
  local cb = s:newGroup()  cb.name = "cb"
  local d  = s:newGroup()   d.name = "d"

  d.parent = ca
  ca.parent = c
  cb.parent = c
  c.parent = b
  b.parent = a

  assert(#a.layers == 1)
  assert(#c.layers == 2)

  app.range.layers = { b }
  app.command.RemoveLayer()
  assert(#a.layers == 0)
  app.undo()
  assert(#a.layers == 1)
  assert(#c.layers == 2)

  -- Crash selecting a layer that was removed and then brought back to
  -- life with "undo"
  app.range.layers = { b }
end

-- Test layer name bug when iterating over layers where names given to
-- some layers are integers and also are valid ranges/indexes in the
-- list of layers
do
  local layerNames = { "2", "4", "Non-integer", "1" }
  local s = Sprite(4, 4)
  local layer1 = s.layers[1]   layer1.name = layerNames[1]
  local layer2 = s:newLayer()  layer2.name = layerNames[2]
  local layer3 = s:newLayer()  layer3.name = layerNames[3]
  local layer4 = s:newLayer()  layer4.name = layerNames[4]

  local i = 1
  for index, layer in ipairs(app.activeSprite.layers) do
    assert(index == i)
    assert(layer.name == layerNames[i])
    i = i + 1
  end
end

-- Compare layers vs sprites (just return false)

do
  local s = Sprite(2, 2)
  assert(s.layers[1].parent == s)
  assert(s.layers[1] ~= s) -- Uses Layer_eq() to compare
  assert(s ~= s.layers[1]) -- Uses Sprite_eq() to compare
end

-- Try to remove all layers and check that it wasn't allowed

do
  -- Try removing default layer
  local s = Sprite(2, 2)
  assert(#s.layers == 1)
  app.command.RemoveLayer()
  assert(#s.layers == 1)

  -- Try deleting all layers when there are more than one.
  s = Sprite(2, 2)
  local a = s.layers[1]
  local b = s:newLayer()
  local c = s:newLayer()
  app.range.layers = { a, b, c }
  assert(#s.layers == 3)
  app.command.RemoveLayer()
  assert(#s.layers == 3)
end
