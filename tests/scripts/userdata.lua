-- Copyright (C) 2022-2025  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  local spr = Sprite(1, 1)

  spr.properties.a = true
  spr.properties.b = 1
  spr.properties.c = "hi"
  spr.properties.d = 2.3
  assert(spr.properties.a == true)
  assert(spr.properties.b == 1)
  assert(spr.properties.c == "hi")
  -- TODO we don't have too much precision saving fixed points in properties
  assert(math.abs(spr.properties.d - 2.3) < 0.00001)
  assert(#spr.properties == 4)

  -- Iterate all properties
  local t = {}
  for k,v in pairs(spr.properties) do
    t[k] = v
  end
  assert(t.a == true)
  assert(t.b == 1)
  assert(t.c == "hi")
  assert(math.abs(t.d - 2.3) < 0.00001)

  spr.properties.a = false
  assert(spr.properties.a == false)

  spr.properties.b = nil
  assert(spr.properties.b == nil)
  assert(#spr.properties == 3)
  -- We set an unexistent property now, undo it, the property shouldn't exist
  app.transaction(function()
    spr.properties.b = 5
  end)
  assert(spr.properties.b == 5)
  assert(#spr.properties == 4)
  app.undo()
  assert(#spr.properties == 3)

  spr.properties.v = { 10, 20, 30 }
  assert(#spr.properties.v == 3)
  assert(spr.properties.v[1] == 10)
  assert(spr.properties.v[2] == 20)
  assert(spr.properties.v[3] == 30)

  spr.properties.u = spr.properties.v -- Copy a property
  assert(#spr.properties.u == 3)
  assert(spr.properties.u[1] == 10)
  assert(spr.properties.u[2] == 20)
  assert(spr.properties.u[3] == 30)

  spr.properties.m = { a=10,
                       b="bye",
                       c={ "a", "b", "c" },
                       d={ a=1, b="d", c={ x=2, y=0.5, z=0 } },
                       e=Point(32, 20),
                       f=Size(40, 80),
                       g=Rectangle(2, 4, 6, 8) }
  assert(#spr.properties == 6)
  local m = spr.properties.m
  assert(m.a == 10)
  assert(m.b == "bye")
  assert(m.c[1] == "a")
  assert(m.c[2] == "b")
  assert(m.c[3] == "c")
  assert(m.d.a == 1)
  assert(m.d.b == "d")
  assert(m.d.c.x == 2)
  assert(m.d.c.y == 0.5)
  assert(m.d.c.z == 0)
  assert(m.e.x == 32)
  assert(m.e.y == 20)
  assert(m.f.width == 40)
  assert(m.f.height == 80)
  assert(m.g.x == 2)
  assert(m.g.y == 4)
  assert(m.g.width == 6)
  assert(m.g.height == 8)

  -- Set all properties
  app.transaction(function()
    spr.properties = { a=1000 }
  end)
  assert(spr.properties.a == 1000)
  assert(#spr.properties == 1)
  app.undo()                    -- Test undo/redo
  assert(spr.properties.a == false)
  assert(#spr.properties == 6)
  app.redo()
  assert(spr.properties.a == 1000)
  assert(#spr.properties == 1)

  -- Extension properties
  spr.properties.a = 10
  spr.properties("ext1").a = 20
  assert(spr.properties.a == 10)
  assert(spr.properties("ext1").a == 20)

  app.transaction(function()
    spr.properties("ext1", { a=30, b=35 })
  end)
  assert(spr.properties("ext1").a == 30)
  assert(spr.properties("ext1").b == 35)
  app.undo()                    -- Test undo/redo
  assert(spr.properties("ext1").a == 20)
  assert(spr.properties("ext1").b == nil)
  app.redo()

  local ext1 = spr.properties("ext1")
  ext1.a = 40
  ext1.b = 45
  assert(spr.properties("ext1").a == 40)
  assert(spr.properties("ext1").b == 45)

  spr.properties("", { a=50, b=60 }) -- Empty extension is the user properties
  assert(spr.properties.a == 50)
  assert(spr.properties.b == 60)

  -- Test undo/redo setting/removing one property at a time
  app.transaction(function()
    spr.properties.a = "hi"
  end)
  assert(spr.properties.a == "hi")
  assert(spr.properties.b == 60)
  app.undo()
  assert(spr.properties.a == 50)
  assert(spr.properties.b == 60)
  app.redo()
  assert(spr.properties.a == "hi")
  assert(spr.properties.b == 60)
  assert(#spr.properties == 2)
  app.transaction(function()
    spr.properties.a = nil
  end)
  assert(#spr.properties == 1)
  assert(spr.properties.a == nil)
  assert(spr.properties.b == 60)
  app.undo()
  assert(#spr.properties == 2)
  assert(spr.properties.a == "hi")
  assert(spr.properties.b == 60)
end

-- Test crash setting nil user data
-- https://github.com/aseprite/aseprite/issues/4187
do
  local spr = Sprite(1, 1)
  spr.data = nil
end

-- Test crash setting the same properties
-- https://github.com/aseprite/aseprite/issues/5519
do
  local spr = Sprite(1, 1)
  local cel = app.cel
  spr.properties = { a=2 }
  cel.properties = spr.properties
  assert(spr.properties.a == 2)
  assert(cel.properties.a == 2)

  -- Do nothing (doesn't add an undo action, doesn't crash)
  cel.properties = cel.properties
  assert(0 == spr.undoHistory.undoSteps)
  assert(1 == #cel.properties)
end

-- Test mixing property changes with undo and without undo
do
  local spr = Sprite(1, 1)
  spr.properties.a = 1
  assert(spr.properties.a == 1)

  app.transaction(function()
    spr.properties.a = 2
  end)
  assert(spr.properties.a == 2)

  spr.properties.a = 3
  assert(spr.properties.a == 3)

  app.undo()
  assert(spr.properties.a == 1)
  app.redo()
  assert(spr.properties.a == 3)

  -- Change all properties
  assert(#spr.properties == 1)
  assert(spr.properties.a == 3)
  app.transaction(function()
    spr.properties = { x=1, y=2 }
  end)
  assert(#spr.properties == 2)
  assert(spr.properties.x == 1)
  assert(spr.properties.y == 2)
  app.undo()
  assert(#spr.properties == 1)
  assert(spr.properties.a == 3)
  spr.properties = { b=2 }
  app.redo()
  assert(#spr.properties == 2)
  app.undo()
  assert(#spr.properties == 1)
  assert(spr.properties.b == 2)
end
