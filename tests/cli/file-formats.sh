#! /bin/bash
# Copyright (C) 2019  Igara Studio S.A.

# Create a simple image and save it in all formats using scripts
d=$t/file-formats
mkdir $d
cat >$d/gen.lua <<EOF
local a = Sprite(32, 32)
app.useTool{ points={{0,0},{31,31}}, tool='filled_ellipse', color=Color(255,255,255) }
a:saveAs('$d/test.aseprite')
a:saveAs('$d/test.gif')
a:saveAs('$d/test.ico')
a:saveAs('$d/test.jpg')
a:saveAs('$d/test.pcx')
a:saveAs('$d/test.svg')
a:saveAs('$d/test.tga')
a:saveAs('$d/test.webp')
EOF
$ASEPRITE -b -script "$d/gen.lua" || exit 1

# Load image and save in all formats using CLI
$ASEPRITE -b "$d/test.aseprite" \
          -save-as "$d/test2.gif" \
          -save-as "$d/test2.ico" \
          -save-as "$d/test2.jpg" \
          -save-as "$d/test2.pcx" \
          -save-as "$d/test2.svg" \
          -save-as "$d/test2.tga" \
          -save-as "$d/test2.webp" \
    || exit 1

# Compare that all images are the same/equivalent
# Note: app.open() and Sprite{fromFile:} are the same
cat >$d/compare.lua <<EOF
local a = Sprite{ fromFile="$d/test.aseprite" }
local b = {
  app.open("$d/test.gif"), app.open("$d/test2.gif"),
  app.open("$d/test.ico"), app.open("$d/test2.ico"),
  app.open("$d/test.jpg"), app.open("$d/test2.jpg"),
  app.open("$d/test.pcx"), app.open("$d/test2.pcx"),
  --app.open("$d/test.svg"), -- we don't support loading SVG files
  app.open("$d/test.tga"), app.open("$d/test2.tga"),
  app.open("$d/test.webp"), app.open("$d/test2.webp")
}
for i,c in ipairs(b) do
  if c.colorMode == ColorMode.INDEXED then
    app.activeSprite = c
    app.command.ChangePixelFormat{ format="rgb" }
  end
  assert(c.colorMode == ColorMode.RGB)
  if c.layers[1].isBackground then   -- jpg and pcx
    app.activeSprite = c
    app.command.LayerFromBackground()
    -- tolerance is 1 to remove jpg noise
    app.command.ReplaceColor{ from=Color(0, 0, 0, 255), to=Color(0, 0, 0, 0), tolerance=1 }
    app.command.ReplaceColor{ from=Color(255, 255, 255, 255), to=Color(255, 255, 255, 255), tolerance=1 }
  end
  assert(a.cels[1].image:isEqual(c.cels[1].image))
end
EOF
$ASEPRITE -b -script "$d/compare.lua" || exit 1
