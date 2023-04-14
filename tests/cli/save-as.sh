#! /bin/bash
# Copyright (C) 2018-2023 Igara Studio S.A.

function list_files() {
    oldwd=$(pwd $PWDARG)
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

# --save-as without path
# https://github.com/aseprite/aseprite/issues/591

d=$t/save-as-without-path
mkdir $d
oldwd=$(pwd $PWDARG)
cd $d
$ASEPRITE -b -split-layers $oldwd/sprites/abcd.aseprite -save-as issue591.png || exit 1
if [[ ! -f "issue591 (a).png" ||
      ! -f "issue591 (b).png" ||
      ! -f "issue591 (c).png" ||
      ! -f "issue591 (d).png" ]]; then
    echo "FAIL: Regression detected (issue 591)"
    exit 1
fi
cd $oldwd

# --save-as group without showing hidden children
# https://github.com/aseprite/aseprite/issues/2084#issuecomment-525835889

if [[ "$(uname)" =~ "MINGW" ]] || [[ "$(uname)" =~ "MSYS" ]] ; then
    # Ignore this test on Windows because we cannot give * as a parameter (?)
    echo Do nothing
else
d=$t/save-as-groups-and-hidden
mkdir $d
$ASEPRITE -b sprites/groups2.aseprite -layer \* -save-as "$d/g2-all.png" || exit 1
$ASEPRITE -b sprites/groups2.aseprite -layer \* -ignore-layer items -save-as "$d/g2-all-without-items.png" || exit 1
$ASEPRITE -b sprites/groups2.aseprite -layer \* -ignore-layer gun -save-as "$d/g2-all-without-gun1.png" || exit 1
$ASEPRITE -b sprites/groups2.aseprite -layer \* -ignore-layer sword -save-as "$d/g2-all-without-sword1.png" || exit 1
$ASEPRITE -b sprites/groups2.aseprite -layer \* -ignore-layer items/gun -save-as "$d/g2-all-without-gun2.png" || exit 1
$ASEPRITE -b sprites/groups2.aseprite -layer \* -ignore-layer items/sword -save-as "$d/g2-all-without-sword2.png" || exit 1
$ASEPRITE -b sprites/groups2.aseprite -layer \* -ignore-layer player -save-as "$d/g2-all-without-player.png" || exit 1
$ASEPRITE -b sprites/groups2.aseprite -layer player -save-as "$d/g2-player.png" || exit 1
$ASEPRITE -b sprites/groups2.aseprite -layer items -save-as "$d/g2-items.png" || exit 1
$ASEPRITE -b sprites/groups2.aseprite -layer items/\* -save-as "$d/g2-items-all.png" || exit 1
$ASEPRITE -b sprites/groups2.aseprite -layer sword -save-as "$d/g2-sword.png" || exit 1
$ASEPRITE -b sprites/groups2.aseprite -layer gun -save-as "$d/g2-gun.png" || exit 1

$ASEPRITE -b sprites/groups3abc.aseprite -layer a -save-as "$d/g3-a.png" || exit 1
$ASEPRITE -b sprites/groups3abc.aseprite -layer b -save-as "$d/g3-b.png" || exit 1
$ASEPRITE -b sprites/groups3abc.aseprite -layer c -save-as "$d/g3-c.png" || exit 1
$ASEPRITE -b sprites/groups3abc.aseprite -layer a/\* -save-as "$d/g3-a-all.png" || exit 1
$ASEPRITE -b sprites/groups3abc.aseprite -layer b/\* -save-as "$d/g3-b-all.png" || exit 1
$ASEPRITE -b sprites/groups3abc.aseprite -layer c/\* -save-as "$d/g3-c-all.png" || exit 1
$ASEPRITE -b sprites/groups3abc.aseprite -layer a/a -save-as "$d/g3-aa.png" || exit 1
$ASEPRITE -b sprites/groups3abc.aseprite -layer b/a -save-as "$d/g3-ba.png" || exit 1
$ASEPRITE -b sprites/groups3abc.aseprite -layer c/a -save-as "$d/g3-ca.png" || exit 1
$ASEPRITE -b sprites/groups3abc.aseprite -layer a/b -save-as "$d/g3-ab.png" || exit 1
$ASEPRITE -b sprites/groups3abc.aseprite -layer b/b -save-as "$d/g3-bb.png" || exit 1
$ASEPRITE -b sprites/groups3abc.aseprite -layer c/b -save-as "$d/g3-cb.png" || exit 1
$ASEPRITE -b sprites/groups3abc.aseprite -layer a/c -save-as "$d/g3-ac.png" || exit 1
$ASEPRITE -b sprites/groups3abc.aseprite -layer b/c -save-as "$d/g3-bc.png" || exit 1
$ASEPRITE -b sprites/groups3abc.aseprite -layer c/c -save-as "$d/g3-cc.png" || exit 1

cat >$d/compare.lua <<EOF
dofile('scripts/test_utils.lua')

local g2 = app.open("sprites/groups2.aseprite")
local g3 = app.open("sprites/groups3abc.aseprite")

function img(name)
  local i = Image{ fromFile="$d/" .. name .. ".png" }
  if not i then error('file ' .. name .. '.png does not exist') end
  return i
end

expect_rendered_layers(img("g2-all"), g2, { "items/gun", "items/sword", "player/head", "player/body" })
expect_rendered_layers(img("g2-all-without-items"), g2, { "player/head", "player/body" })
expect_rendered_layers(img("g2-all-without-gun1"), g2, { "items/sword", "player/head", "player/body" })
expect_rendered_layers(img("g2-all-without-gun2"), g2, { "items/sword", "player/head", "player/body" })
expect_rendered_layers(img("g2-all-without-sword1"), g2, { "items/gun", "player/head", "player/body" })
expect_rendered_layers(img("g2-all-without-sword2"), g2, { "items/gun", "player/head", "player/body" })
expect_rendered_layers(img("g2-player"), g2, { "player/head", "player/body" })
expect_rendered_layers(img("g2-items"), g2, { "items/sword" })
expect_rendered_layers(img("g2-items-all"), g2, { "items/gun", "items/sword" })
expect_rendered_layers(img("g2-sword"), g2, { "items/sword" })
expect_rendered_layers(img("g2-gun"), g2, { "items/gun" })

expect_rendered_layers(img("g3-a"), g3, { "a/a", "a/b", "a/c" })
expect_rendered_layers(img("g3-b"), g3, { "b/a", "b/c" })
expect_rendered_layers(img("g3-c"), g3, { "c/c" })
expect_rendered_layers(img("g3-a-all"), g3, { "a/a", "a/b", "a/c" })
expect_rendered_layers(img("g3-b-all"), g3, { "b/a", "b/b", "b/c" })
expect_rendered_layers(img("g3-c-all"), g3, { "c/a", "c/b", "c/c" })
expect_rendered_layers(img("g3-aa"), g3, { "a/a" })
expect_rendered_layers(img("g3-ab"), g3, { "a/b" })
expect_rendered_layers(img("g3-ac"), g3, { "a/c" })
expect_rendered_layers(img("g3-ba"), g3, { "b/a" })
expect_rendered_layers(img("g3-bb"), g3, { "b/b" })
expect_rendered_layers(img("g3-bc"), g3, { "b/c" })
expect_rendered_layers(img("g3-ca"), g3, { "c/a" })
expect_rendered_layers(img("g3-cb"), g3, { "c/b" })
expect_rendered_layers(img("g3-cc"), g3, { "c/c" })
EOF
$ASEPRITE -b -script "$d/compare.lua" || exit 1
fi

# Test --save-as {title}
# https://github.com/aseprite/aseprite/issues/2442
# https://community.aseprite.org/t/16491

d=$t/save-as-title
$ASEPRITE -b sprites/groups2.aseprite -save-as "$d/{title}.png" || exit 1
$ASEPRITE -b sprites/groups3abc.aseprite -save-as "$d/{title}.png" || exit 1
$ASEPRITE -b sprites/link.aseprite -save-as "$d/{title}{frame}.png" || exit 1
expect "groups2.png
groups3abc.png
link0.png
link1.png" "list_files $d"

d=$t/save-as-title-multiplefiles
$ASEPRITE -b sprites/groups2.aseprite \
             sprites/groups3abc.aseprite \
             sprites/link.aseprite -save-as "$d/{title}-{frame1}.png" || exit 1
expect "groups2-1.png
groups3abc-1.png
link-1.png
link-2.png" "list_files $d"

# Test regression with --save-as {tag}_1.png to interpret 1 as {frame1}
# https://community.aseprite.org/t/17253

d=$t/save-as-1-as-frame1-ok
$ASEPRITE -b sprites/1empty3.aseprite \
             -save-as "$d/{tag}-{frame1}.png" || exit 1
expect "a-1.png
a-2.png
b-1.png" "list_files $d"

d=$t/save-as-1-as-frame1-regression
$ASEPRITE -b sprites/1empty3.aseprite \
             -save-as "$d/{tag}-1.png" || exit 1
expect "a-1.png
a-2.png
b-1.png" "list_files $d"

# Test that -save-as {tag}.png will save with the frame number when needed

d=$t/save-as-with-format-but-without-frame
$ASEPRITE -b sprites/1empty3.aseprite \
             -save-as "$d/{tag}.png" || exit 1
expect "a1.png
a2.png
b.png" "list_files $d"

# Test https://github.com/aseprite/aseprite/issues/3733#issuecomment-1489720933
#      https://github.com/aseprite/aseprite/issues/3733#issuecomment-1494682407
# Same as previous test, but with -filename-format with -split-tags

d=$t/save-as-with-filename-format--and-split-tags-without-frame
$ASEPRITE -b sprites/1empty3.aseprite \
          -split-tags -filename-format "$d/{tag}.png" \
          -save-as "$d/test.png"
expect "a1.png
a2.png
b.png" "list_files $d"

# Regression with -save-as {slice}
# https://github.com/aseprite/aseprite/issues/3801

d=$t/save-as-with-slice
$ASEPRITE -b sprites/slices.aseprite -save-as $d/{slice}.png
expect "line.png
square.png" "list_files $d"
