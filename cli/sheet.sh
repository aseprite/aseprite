#! /bin/bash
# Copyright (C) 2019-2020 Igara Studio S.A.

# $1 = first sprite sheet json file
# $2 = second sprite sheet json file
function compare_sheet_data() {
    cat $1 | grep -v "\"image\"" > $1-tmp
    cat $2 | grep -v "\"image\"" > $2-tmp
    diff -u $1-tmp $2-tmp
}

# --sheet and STDOUT

d=$t/sheet
mkdir $d # we need to create the directory because the >STDOUT redirection
if ! $ASEPRITE -b sprites/1empty3.aseprite --sheet "$d/sheet.png" > "$d/stdout.json" ; then
   exit 1
fi
cat >$d/compare.lua <<EOF
local json = dofile('third_party/json/json.lua')
local data = json.decode(io.open('$d/stdout.json'):read('a'))
local frames = { data.frames['1empty3 0.aseprite'],
                 data.frames['1empty3 1.aseprite'],
                 data.frames['1empty3 2.aseprite'] }
assert(frames[1].frame.x == 0)
assert(frames[2].frame.x == 32)
assert(frames[3].frame.x == 64)
for i,v in ipairs(frames) do
  assert(v.frame.y == 0)
  assert(v.frame.w == 32)
  assert(v.frame.h == 32)
end
EOF
$ASEPRITE -b -script "$d/compare.lua" || exit 1

# --sheet --data

d0=$d
d=$t/sheet-data
$ASEPRITE -b sprites/1empty3.aseprite --sheet "$d/sheet.png" --data "$d/sheet.json" || exit 1
compare_sheet_data $d0/stdout.json $d/sheet.json || exit 1
cat >$d/compare.lua <<EOF
local orig = app.open("sprites/1empty3.aseprite")
local sheet = app.open("$d/sheet.png")
local expected = Image(orig.width*3, orig.height, orig.colorMode)
expected:clear()
for f = 1,3 do
  expected:drawSprite(orig, f, (f-1)*orig.width, 0)
end
assert(expected:isEqual(sheet.cels[1].image))
EOF
$ASEPRITE -b -script "$d/compare.lua" || exit 1

# vertical sprite sheet

d=$t/vertical-sheet
mkdir $d
# TODO this should be possible in a future
# $ASEPRITE -b sprites/1empty3.aseprite -sheet-type vertical --sheet "$d/sheet.png" --data "$d/sheet.json" || exit 1
cat >$d/create.lua <<EOF
local sprite = app.open("sprites/1empty3.aseprite")
app.command.ExportSpriteSheet {
  type="vertical",
  textureFilename="$d/sheet1.png",
  dataFilename="$d/sheet1.json"
}
app.command.ExportSpriteSheet {
  type=SpriteSheetType.VERTICAL,
  textureFilename="$d/sheet2.png",
  dataFilename="$d/sheet2.json"
}
EOF
$ASEPRITE -b -script "$d/create.lua" || exit 1
compare_sheet_data "$d/sheet1.json" "$d/sheet2.json" || exit 1
cmp "$d/sheet1.png" "$d/sheet2.png" || exit 1
cat >$d/compare.lua <<EOF
local orig = app.open("sprites/1empty3.aseprite")
local sheet = app.open("$d/sheet1.png")
local expected = Image(orig.width, orig.height*3, orig.colorMode)
expected:clear()
for f = 1,3 do
  expected:drawSprite(orig, f, 0, (f-1)*orig.height)
end
assert(expected:isEqual(sheet.cels[1].image))
EOF
$ASEPRITE -b -script "$d/compare.lua" || exit 1

# --split-layers --sheet --data

d=$t/split-layers-sheet-data
$ASEPRITE -b --split-layers sprites/1empty3.aseprite \
	  --filename-format "{layer}-{frame}" \
	  --sheet "$d/sheet.png" \
	  --data "$d/sheet.json" || exit 1
cat >$d/compare.lua <<EOF
local json = dofile('third_party/json/json.lua')
local data = json.decode(io.open('$d/sheet.json'):read('a'))
assert(data.meta.size.w == 96)
assert(data.meta.size.h == 64)

local orig = app.open("sprites/1empty3.aseprite")
local sheet = app.open("$d/sheet.png")
local expected = Image(orig.width*3, orig.height*2, orig.colorMode)
expected:clear()
for lay = 1,2 do
  orig.layers[1].isVisible = (lay == 1)
  orig.layers[2].isVisible = (lay == 2)
  for frm = 1,3 do
    local x, y = (frm-1)*orig.width, (lay-1)*orig.height
    expected:drawSprite(orig, frm, x, y)

    local frmData = data.frames[orig.layers[lay].name .. '-' .. (frm-1)]
    assert(frmData.frame.x == x)
    assert(frmData.frame.y == y)
    assert(frmData.frame.w == 32)
    assert(frmData.frame.h == 32)
  end
end
assert(expected:isEqual(sheet.cels[1].image))
EOF
$ASEPRITE -b -script "$d/compare.lua" || exit 1

# Test that the transparent color persists in the output sheet

d=$t/sheet-custom-transparent-index
if ! $ASEPRITE -b sprites/bg-index-3.aseprite -sheet "$d/sheet.aseprite" -data "$d/sheet.json" ; then
    exit 1
fi
cat >$d/compare.lua <<EOF
local original = Sprite{ fromFile='sprites/bg-index-3.aseprite' }
assert(original.transparentColor == 3)
local sheet = Sprite{ fromFile='$d/sheet.aseprite' }
assert(sheet.transparentColor == 3)
EOF
$ASEPRITE -b -script "$d/compare.lua" || exit 1

# Don't discard empty frames if -ignore-empty is not used (even if -trim is used)
# https://github.com/aseprite/aseprite/issues/2116
# -layer -trim -ignore-empty -list-tags -sheet -data

d=$t/sheet-trim-without-ignore-empty
$ASEPRITE -b \
	  -list-tags \
	  -layer "c" \
	  "sprites/tags3.aseprite" \
	  -trim \
	  -sheet-pack \
	  -sheet "$d/sheet1.png" \
	  -format json-array \
	  -data "$d/sheet1.json" || exit 1
$ASEPRITE -b \
	  -list-tags \
	  -layer "c" \
	  "sprites/tags3.aseprite" \
	  -trim -ignore-empty \
	  -sheet-pack \
	  -sheet "$d/sheet2.png" \
	  -format json-array \
	  -data "$d/sheet2.json" || exit 1

cat >$d/check.lua <<EOF
local json = dofile('third_party/json/json.lua')
local sheet1 = json.decode(io.open('$d/sheet1.json'):read('a'))
local sheet2 = json.decode(io.open('$d/sheet2.json'):read('a'))
assert(#sheet1.frames == 12)
assert(#sheet2.frames == 4)
EOF
$ASEPRITE -b -script "$d/check.lua" || exit 1

# -sheet -sheet-columns vs -sheet-rows

d=$t/sheet-columns-and-rows
$ASEPRITE -b -split-layers sprites/1empty3.aseprite \
	  -filename-format "{layer}{frame}" \
	  -sheet "$d/sheet1.png" \
	  -sheet-type rows \
	  -sheet-columns 3 \
	  -data "$d/sheet1.json" || exit $?
$ASEPRITE -b -split-layers sprites/1empty3.aseprite \
	  -filename-format "{layer}{frame}" \
	  -sheet "$d/sheet2.png" \
	  -sheet-type columns \
	  -sheet-rows 3 \
	  -data "$d/sheet2.json" || exit $?
$ASEPRITE -b \
	  -script-param file1=$d/sheet1.json \
	  -script-param file2=$d/sheet2.json \
	  -script scripts/compare_sprite_sheets.lua || exit $?

# -sheet -trim vs -trim-sprite

d=$t/sheet-columns-and-rows
$ASEPRITE -b -split-layers sprites/1empty3.aseprite \
	  -trim \
	  -filename-format "{layer}{frame}" \
	  -sheet "$d/sheet1.png" \
	  -data "$d/sheet1.json" || exit $?

$ASEPRITE -b -split-layers sprites/1empty3.aseprite \
	  -trim-sprite \
	  -filename-format "{layer}{frame}" \
	  -sheet "$d/sheet2.png" \
	  -data "$d/sheet2.json" || exit $?
$ASEPRITE -b \
	  -script-param file1=$d/sheet1.json \
	  -script-param file2=$d/sheet2.json \
	  -script scripts/compare_sprite_sheets.lua || exit $?

# Test all sprite sheet types
# -sheet horizontal/vertical/rows/columns/packed
d=$t/sheet-all-types
for type in horizontal vertical rows columns packed ; do
    $ASEPRITE -b "sprites/tags3.aseprite" \
	      -sheet-type $type -sheet "$d/$type.png" \
	      -format json-array -data "$d/$type.json" || exit $?

    $ASEPRITE -b -split-layers "sprites/tags3.aseprite" \
	      -sheet-type $type -sheet "$d/$type-layers.png" \
	      -format json-array -data "$d/$type-layers.json" || exit $?

    $ASEPRITE -b -split-layers -merge-duplicates "sprites/tags3.aseprite" \
	      -sheet-type $type -sheet "$d/$type-layers-merge-duplicates.png" \
	      -format json-array -data "$d/$type-layers-merge-duplicates.json" || exit $?

    $ASEPRITE -b -split-tags "sprites/tags3.aseprite" \
	      -sheet-type $type -sheet "$d/$type-tags.png" \
	      -format json-array -data "$d/$type-tags.json" || exit $?

    $ASEPRITE -b -split-tags -trim "sprites/tags3.aseprite" \
	      -sheet-type $type -sheet "$d/$type-tags-trim.png" \
	      -format json-array -data "$d/$type-tags-trim.json" || exit $?

    $ASEPRITE -b -split-layers -split-tags "sprites/tags3.aseprite" \
	      -sheet-type $type -sheet "$d/$type-layer-tags.png" \
	      -format json-array -data "$d/$type-layer-tags.json" || exit $?
done

for type in horizontal vertical rows columns ; do
    for subtype in "" "-layers" "-layers-merge-duplicates" "-tags" "-tags-trim" "-layer-tags" ; do
	$ASEPRITE -b \
		  -script-param file1=$d/packed$subtype.json \
		  -script-param file2=$d/$type$subtype.json \
		  -script scripts/compare_sprite_sheets.lua || exit $?
    done
done

# "Trim Cels" (-trim) with -merge-duplicates didn't generate the
# correct "spriteSourceSize" for each frame.
# https://igarastudio.zendesk.com/agent/tickets/407
d=$t/ticket-407
for layer in a b ; do
    $ASEPRITE -b -layer "$layer" "sprites/point4frames.aseprite" \
	      -trim \
	      -data "$d/data1-$layer.json" \
	      -format json-array -sheet "$d/sheet1-$layer.png" || exit 1
    $ASEPRITE -b -layer "$layer" "sprites/point4frames.aseprite" \
	      -trim -merge-duplicates \
	      -data "$d/data2-$layer.json" \
	      -format json-array -sheet "$d/sheet2-$layer.png" || exit 1
    cat >$d/compare.lua <<EOF
local json = dofile('third_party/json/json.lua')
local data1 = json.decode(io.open('$d/data1-$layer.json'):read('a'))
local data2 = json.decode(io.open('$d/data2-$layer.json'):read('a'))
for i = 1,4 do
    local a = data1.frames[i].spriteSourceSize
    local b = data2.frames[i].spriteSourceSize
    assert(a.x == b.x)
    assert(a.y == b.y)
    assert(a.w == b.w)
    assert(a.h == b.h)
end
EOF
    $ASEPRITE -b -script "$d/compare.lua" || exit 1
done
