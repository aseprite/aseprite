// Copyright (C) 2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

var s = new Sprite(16, 32)
console.assert(s.width == 16)
console.assert(s.height == 32)

s.width = 20
console.assert(s.width == 20)
console.assert(s.height == 32)

s.height = 40
console.assert(s.width == 20)
console.assert(s.height == 40)

app.undo()
console.assert(s.width == 20)
console.assert(s.height == 32)

app.undo()
console.assert(s.width == 16)
console.assert(s.height == 32)

app.transaction(
    function() {
        s.width = 20
        s.height = 40
    })
console.assert(s.width == 20)
console.assert(s.height == 40)

app.undo()
console.assert(s.width == 16)
console.assert(s.height == 32)

app.redo()
console.assert(s.width == 20)
console.assert(s.height == 40)
