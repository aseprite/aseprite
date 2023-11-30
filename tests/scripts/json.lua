-- Copyright (C) 2023  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

-- Basic decode + iterators
do
  local o = json.decode('{"a":true,"b":5,"c":[1,3,9]}')
  assert(#o == 3)
  assert(o.a == true)
  assert(o.b == 5)
  assert(#o.c == 3)
  assert(o.c[1] == 1)
  assert(o.c[2] == 3)
  assert(o.c[3] == 9)

  -- Iterate json data
  for k,v in pairs(o) do
    if k == "a" then assert(v == true) end
    if k == "b" then assert(v == 5) end
    if k == "c" then
      assert(v[1] == 1)
      assert(v[2] == 3)
      assert(v[3] == 9)
    end
  end

  for i,v in ipairs(o.c) do
    if i == 1 then assert(v == 1) end
    if i == 2 then assert(v == 3) end
    if i == 3 then assert(v == 9) end
  end
end

-- Modify values
do
  local o = json.decode('{"obj":{"a":3,"b":5},"arr":[0,"hi",{"bye":true}]}')
  assert(o.obj.a == 3)
  assert(o.obj.b == 5)
  assert(o.arr[1] == 0)
  assert(o.arr[2] == "hi")
  assert(o.arr[3].bye == true)

  local u = json.decode(json.encode(o)) -- Creates a copy
  assert(o == o)
  assert(o == u)

  o.obj.b = 6
  o.arr[1] = "name"
  assert(tostring(o.obj) == '{"a": 3, "b": 6}')
  assert(tostring(o.arr) == '["name", "hi", {"bye": true}]')
  -- Check that "u" is a copy and wasn't modify (only "o" was modified)
  assert(tostring(u.obj) == '{"a": 3, "b": 5}')
  assert(tostring(u.arr) == '[0, "hi", {"bye": true}]')
end

-- Encode Lua tables
do
  local arrStr = json.encode({ 4, "hi", true })
  assert(arrStr == '[4, "hi", true]')
  local arr = json.decode(arrStr)
  assert(arr[1] == 4)
  assert(arr[2] == "hi")
  assert(arr[3] == true)

  local obj = json.decode(json.encode({ a=4, b=true, c="name", d={1,8,{a=2}} }))
  assert(obj.a == 4)
  assert(obj.b == true)
  assert(obj.c == "name")
  assert(obj.d[1] == 1)
  assert(obj.d[2] == 8)
  assert(obj.d[3].a == 2)
end

-- Test crash setting fields (index out of bounds, or setting a field
-- in an array object, etc.).
-- https://github.com/aseprite/aseprite/issues/4166
do
  local o = json.decode('{"a":[10,20,30]}')
  assert(#o == 1)
  assert(#o.a == 3)
  assert(o.a[1] == 10)
  assert(o.a[2] == 20)
  assert(o.a[3] == 30)
  assert(o.a[4] == nil)
  assert(o.a["b"] == nil)

  o.a[4] = 40
  assert(#o.a == 4)
  assert(o.a[4] == 40)

  -- Cannot add a map field to an array
  o.a.b = "d"
  assert(o.a.b == nil)

  -- Creating a field that is an object and set a field
  o.b = { c=1 }
  o.b.d = 2
  assert(o.b.c == 1)
  assert(o.b.d == 2)

  assert(tostring(o) == '{"a": [10, 20, 30, 40], "b": {"c": 1, "d": 2}}')
end
