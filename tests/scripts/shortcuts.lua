-- Copyright (C) 2025  João Francisco Guarda Pozzer - Fellow Roach
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

dofile('./test_utils.lua')

-- Test shortcut format parsing (verify 2x/3x prefix pattern)
do
  local test_patterns = {
    { str = "2x+X", has_2x = true, has_3x = false },
    { str = "3x+X", has_2x = false, has_3x = true },
    { str = "2x+Ctrl+S", has_2x = true, has_3x = false },
    { str = "3x+Shift+A", has_2x = false, has_3x = true },
    { str = "Ctrl+X", has_2x = false, has_3x = false },
    { str = "X", has_2x = false, has_3x = false },
  }
  
  for _, test in ipairs(test_patterns) do
    local has_2x = test.str:match("^2x%+") ~= nil
    local has_3x = test.str:match("^3x%+") ~= nil
    expect_eq(has_2x, test.has_2x)
    expect_eq(has_3x, test.has_3x)
    -- Verify mutually exclusive
    assert(not (has_2x and has_3x))
  end
end

-- Test that KeyboardShortcuts command exists
do
  assert(app.command.KeyboardShortcuts ~= nil)
end

-- Test keyboard event API has click modifier fields (UI mode only)
if app.isUIAvailable then
  local dlg = Dialog()
  dlg:canvas{
    width = 32,
    height = 32,
    onkeydown = function(ev)
      -- Verify new click modifier fields exist and are boolean
      assert(type(ev.doubleClickKey) == "boolean")
      assert(type(ev.tripleClickKey) == "boolean")
      
      -- Verify existing modifiers still work
      assert(type(ev.altKey) == "boolean")
      assert(type(ev.ctrlKey) == "boolean")
      assert(type(ev.shiftKey) == "boolean")
      assert(type(ev.spaceKey) == "boolean")
      assert(type(ev.metaKey) == "boolean")
    end
  }
  assert(dlg ~= nil)
  dlg:close()
end
