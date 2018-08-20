// Copyright (C) 2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

var sz = new Size()
console.assert(sz.width == 0)
console.assert(sz.height == 0)

sz = new Size(3, 4)
console.assert(sz.width == 3)
console.assert(sz.height == 4)

var sz2 = new Size(sz)
console.assert(sz2.width == 3)
console.assert(sz2.height == 4)

sz.width = 7;
sz.height = 8;
console.assert(sz.width == 7)
console.assert(sz.height == 8)

// TODO fix this
console.log(JSON.stringify(sz))
