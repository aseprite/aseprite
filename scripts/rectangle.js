// Copyright (C) 2018  David Capello

var rc = new Rectangle()
console.assert(rc.x == 0)
console.assert(rc.y == 0)
console.assert(rc.width == 0)
console.assert(rc.height == 0)

rc = new Rectangle(1, 2, 3, 4)
console.assert(rc.x == 1)
console.assert(rc.y == 2)
console.assert(rc.width == 3)
console.assert(rc.height == 4)

var rc2 = new Rectangle(rc)
console.assert(rc2.x == 1)
console.assert(rc2.y == 2)
console.assert(rc2.width == 3)
console.assert(rc2.height == 4)

rc.x = 5;
rc.y = 6;
rc.width = 7;
rc.height = 8;
console.assert(rc.x == 5)
console.assert(rc.y == 6)
console.assert(rc.width == 7)
console.assert(rc.height == 8)

// TODO fix this
console.log(JSON.stringify(rc))
