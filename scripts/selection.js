// Copyright (C) 2018  David Capello

// Isolated selection
var a = new Selection()
console.assert(a.bounds.x == 0)
console.assert(a.bounds.y == 0)
console.assert(a.bounds.width == 0)
console.assert(a.bounds.height == 0)

a.select(1, 2, 3, 4);
console.assert(a.bounds.x == 1)
console.assert(a.bounds.y == 2)
console.assert(a.bounds.width == 3)
console.assert(a.bounds.height == 4)

a.select({ 'x': 5, 'y': 6, 'width': 7, 'height': 8 })
console.assert(a.bounds.x == 5)
console.assert(a.bounds.y == 6)
console.assert(a.bounds.width == 7)
console.assert(a.bounds.height == 8)

a.deselect();
console.assert(a.bounds.x == 0)
console.assert(a.bounds.y == 0)
console.assert(a.bounds.width == 0)
console.assert(a.bounds.height == 0)

// Selection related to a sprite
// TODO
