#! /bin/bash
# Copyright (C) 2018 Igara Studio S.A.

function list_files() {
    oldwd=$(pwd)
    cd $1 && ls -1 *.*
    cd $oldwd
}

# --save-as

d=$t/save-as
$ASEPRITE -b sprites/1empty3.aseprite --save-as "$d/image00.png" || exit 1
expect "image00.png
image01.png
image02.png" "list_files $d"

# --ignore-empty --save-as

d=$t/save-as-ignore-empty
$ASEPRITE -b sprites/1empty3.aseprite --ignore-empty --save-as $d/image00.png || exit 1
expect "image00.png
image02.png" "list_files $d"

# --split-layers --save-as

d=$t/save-as-split-layers
$ASEPRITE -b sprites/1empty3.aseprite --split-layers --save-as $d/layer.png || exit 1
expect "layer (bg) 0.png
layer (bg) 1.png
layer (bg) 2.png
layer (fg) 0.png
layer (fg) 1.png
layer (fg) 2.png" "list_files $d"

# --save-as {layer}

d=$t/save-as-layer
mkdir $d  # TODO why do we need this?
$ASEPRITE -b sprites/1empty3.aseprite --save-as $d/layer-{layer}.gif || exit 1
expect "layer-bg.gif
layer-fg.gif" "list_files $d"

# --save-as frame8-test.png

d=$t/save-as-frame8-test
$ASEPRITE -b sprites/1empty3.aseprite --save-as "$d/frame8-test.png" || exit 1
expect "frame8-test1.png
frame8-test2.png
frame8-test3.png" "list_files $d"

# --save-as frame-0.png

d=$t/save-as-frame-0
$ASEPRITE -b sprites/1empty3.aseprite --save-as "$d/frame-0.png" || exit 1
expect "frame-0.png
frame-1.png
frame-2.png" "list_files $d"

# --save-as frame-00.png

d=$t/save-as-frame-00
$ASEPRITE -b sprites/1empty3.aseprite --save-as "$d/frame-00.png" || exit 1
expect "frame-00.png
frame-01.png
frame-02.png" "list_files $d"

# --save-as frame-001.png

d=$t/save-as-frame-001
$ASEPRITE -b sprites/1empty3.aseprite --save-as "$d/frame-001.png" || exit 1
expect "frame-001.png
frame-002.png
frame-003.png" "list_files $d"

# --save-as frame-0032.png

d=$t/save-as-frame-0032
$ASEPRITE -b sprites/1empty3.aseprite --save-as "$d/frame-0032.png" || exit 1
expect "frame-0032.png
frame-0033.png
frame-0034.png" "list_files $d"

# --trim --save-as

d=$t/save-as-trim
$ASEPRITE -b --trim sprites/1empty3.aseprite --save-as "$d/trim-000.png" || exit 1
expect "trim-000.png
trim-001.png
trim-002.png" "list_files $d"
cat >$d/compare.lua <<EOF
local a = app.open("sprites/1empty3.aseprite")
assert(a.width == 32)
assert(a.height == 32)
app.command.FlattenLayers()
app.command.AutocropSprite()
assert(a.width == 22)
assert(a.height == 26)
local b = app.open("$d/trim-000.png")
assert(b.width == 22)
assert(b.height == 26)
local cleanImage = Image(b.spec)
cleanImage:clear()
for f = 1,3 do
  local celA = a.layers[1]:cel(f)
  local celB = b.layers[1]:cel(f)
  if celA and celB then
    assert(celA.image:isEqual(celB.image))
  else
    assert(not celA)
    assert(not celB or celB.image:isEqual(cleanImage))
  end
end
EOF
$ASEPRITE -b --script "$d/compare.lua" || exit 1

# -split-layers -trim -save-as

d=$t/save-as-split-layers-trim
$ASEPRITE -batch -split-layers -trim sprites/1empty3.aseprite -save-as "$d/{layer}{frame}.png" || exit 1
expect "bg0.png
bg1.png
bg2.png
fg0.png
fg1.png
fg2.png" "list_files $d"
cat >$d/compare.lua <<EOF
local orig = app.open("sprites/1empty3.aseprite")
local bg = app.open("$d/bg0.png")
local fg = app.open("$d/fg0.png")
assert(bg.width == 22)
assert(bg.height == 11)
assert(fg.width == 19)
assert(fg.height == 11)
for f = 1,3 do
  local origCelBG = orig.layers[1]:cel(f)
  local origCelFG = orig.layers[2]:cel(f)
  local celBG = bg.layers[1]:cel(f)
  local celFG = fg.layers[1]:cel(f)
  if origCelBG and celBG then
    assert(origCelBG.image:isEqual(celBG.image))
  else
    local cleanImage = Image(celBG.image.spec)
    cleanImage:clear()
    assert(not origCelBG)
    assert(celBG.image:isEqual(cleanImage))
  end
  if origCelFG and celFG then
    assert(origCelFG.image:isEqual(celFG.image))
  else
    local cleanImage = Image(celFG.image.spec)
    cleanImage:clear()
    assert(not origCelFG)
    assert(celFG.image:isEqual(cleanImage))
  end
end
EOF
$ASEPRITE -b -script "$d/compare.lua" || exit 1
