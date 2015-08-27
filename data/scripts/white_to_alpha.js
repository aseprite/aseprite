// Aseprite
// Copyright (C) 2015 by David Capello

var spr = activeSprite

for (y=0; y<spr.height; ++y) {
  for (x=0; x<spr.width; ++x) {
    var c = spr.getPixel(x, y)
    var v = (rgbaR(c)+rgbaG(c)+rgbaB(c))/3
    spr.putPixel(x, y, rgba(rgbaR(c),
                            rgbaG(c),
                            rgbaB(c),
                            255-v))
  }
}
