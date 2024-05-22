-- Copyright (C) 2019  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

local fs = app.fs
local sep = fs.pathSeparator

assert('' == fs.filePath('first.png'))
assert('path' == fs.filePath('path/second.png'))
if app.os.windows then
  assert('C:\\path' == fs.filePath('C:\\path\\third.png'))
end

assert('first.png' == fs.fileName('first.png'))
assert('second.png' == fs.fileName('path/second.png'))
if app.os.windows then
  assert('third.png' == fs.fileName('C:\\path\\third.png'))
end

assert('png' == fs.fileExtension('path/file.png'))

assert('first' == fs.fileTitle('first.png'))
assert('second' == fs.fileTitle('path/second.png'))
if app.os.windows then
  assert('third' == fs.fileTitle('C:\\path\\third.png'))
end

assert('first' == fs.filePathAndTitle('first.png'))
assert('path/second' == fs.filePathAndTitle('path/second.png'))
if app.os.windows then
  assert('C:\\path\\third' == fs.filePathAndTitle('C:\\path\\third.png'))
end

assert('hi/bye' == fs.joinPath('hi/', 'bye'))
assert('hi/bye' .. sep .. 'smth.png' == fs.joinPath('hi/', 'bye', 'smth.png'))

local pwd = fs.currentPath
assert(pwd ~= nil)
assert(fs.isDirectory(pwd))
assert(not fs.isFile(pwd))
assert(fs.isFile(fs.joinPath(pwd, 'run-tests.sh')))

do
  local runTestsFound = false
  local readmeFound = false
  local files = fs.listFiles(pwd)
  for i in pairs(files) do
    if files[i] == 'run-tests.sh' then
      runTestsFound = true
    elseif files[i] == 'README.md' then
      readmeFound = true
    end

    local fullFs = fs.joinPath(pwd, files[i])
    if fs.isFile(fullFs) then
      assert(fs.fileSize(fullFs) > 0)
    end
  end
  assert(runTestsFound)
  assert(readmeFound)
end

-- Create directories
do
  assert(fs.makeDirectory("_tmp"))
  assert(fs.isDirectory("_tmp"))

  assert(fs.makeAllDirectories("_tmp/a/b"))
  assert(fs.isDirectory("_tmp/a"))
  assert(fs.isDirectory("_tmp/a/b"))

  assert(fs.removeDirectory("_tmp/a/b"))
  assert(not fs.isDirectory("_tmp/a/b"))
  assert(fs.isDirectory("_tmp/a"))

  assert(not fs.removeDirectory("_tmp")) -- Should fail
  assert(fs.isDirectory("_tmp/a"))
  assert(fs.isDirectory("_tmp"))

  assert(fs.removeDirectory("_tmp/a"))
  assert(fs.removeDirectory("_tmp"))
  assert(not fs.isDirectory("_tmp/a"))
  assert(not fs.isDirectory("_tmp"))
end
