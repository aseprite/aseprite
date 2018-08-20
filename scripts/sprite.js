// Copyright (C) 2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

var a = new Sprite(32, 64)
console.assert(a.width == 32)
console.assert(a.height == 64)
console.assert(a.colorMode == ColorMode.RGB) // RGB by default

var s = a.selection
console.assert(s.bounds.x == 0)
console.assert(s.bounds.y == 0)
console.assert(s.bounds.width == 0)
console.assert(s.bounds.height == 0)

s.selectAll()
console.assert(s.bounds.x == 0)
console.assert(s.bounds.y == 0)
console.assert(s.bounds.width == a.width)
console.assert(s.bounds.height == a.height)

s.select(2, 3, 4, 5)
console.assert(s.bounds.x == 2)
console.assert(s.bounds.y == 3)
console.assert(s.bounds.width == 4)
console.assert(s.bounds.height == 5)

a.crop()
console.assert(a.width == 4)
console.assert(a.height == 5)
a.resize(6, 8)
console.assert(a.width == 6)
console.assert(a.height == 8)
a.crop({ 'x': -1, 'y': -1, 'width': 20, 'height': 30 })
console.assert(a.width == 20)
console.assert(a.height == 30)

// resize sprite setting width/height
a.width = 8
a.height = 10
console.assert(a.width == 8)
console.assert(a.height == 10)

var b = new Sprite(4, 4, ColorMode.INDEXED)
console.assert(b.width == 4)
console.assert(b.height == 4)
console.assert(b.colorMode == ColorMode.INDEXED)
