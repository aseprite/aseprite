-- Copyright (C) 2023  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

if app.isUIAvailable then
  dofile('./test_utils.lua')

  -- Test dialog bounds
  do
    local dlg = Dialog("Bounds test")
    local screenSize = Size(app.window.width, app.window.height)
    local bounds = dlg.bounds
    assert(bounds.x == (math.floor(screenSize.width / 2) - math.floor(bounds.width / 2)))
    assert(bounds.y == (math.floor(screenSize.height / 2) - math.floor(bounds.height / 2)))
    local bounds2 = bounds
    dlg:show { wait=false }
    bounds = dlg.bounds
    assert(bounds == bounds2)
    print(bounds)
    dlg:close()
  end

  do
    local rect = Rectangle(10, 20, 200, 50)
    local dlg2 = Dialog("Bounds test 2")
    dlg2.bounds = rect
    assert(dlg2.bounds == rect)
    dlg2:show { wait=false }
    assert(dlg2.bounds == rect)
    dlg2:close()
  end
end
