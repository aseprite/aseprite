// Copyright (C) 2018  David Capello

var pt = new Point()
console.assert(pt.x == 0)
console.assert(pt.y == 0)

pt = new Point(1, 2)
console.assert(pt.x == 1)
console.assert(pt.y == 2)

var pt2 = new Point(pt)
console.assert(pt2.x == 1)
console.assert(pt2.y == 2)

pt.x = 5;
pt.y = 6;
console.assert(pt.x == 5)
console.assert(pt.y == 6)

// TODO fix this
console.log(JSON.stringify(pt))
