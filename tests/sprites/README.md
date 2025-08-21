# Files

* `abcd.aseprite`: Indexed, 32x32, four layers ("a", "b", "c", "d")
* `1empty3.aseprite`: RGB, 32x32, two layers ("fg", "bg"), 2nd frame
  completelly empty, two tags ("a", "b")
* `groups2.aseprite`: Indexed, 8x8, two groups ("items", "player"),
  two layers per group ("items/gun", "items/sword", "player/head",
  "player/body"), with one layer hidden ("items/gun").
* `groups3abc.aseprite`: RGB, 9x11, three groups ("a", "b", "c"), with
  three layers each one (called "a", "b", "c" too). There is a
  combination of visibilities ("b/b", "c", "c/a", and "c/b" are
  hidden).
* `bg-index-3.aseprite`: Indexed, 4x4, two layers ("fg" and "bg")
  with a special transparent index color (= palette index 3).
* `tags3.aseprite`: 3 tags ("forward", "reverse", "pingpong") and 3
  layers ("a", "b", "c"), 4x4, several linked cels + layer "c" with
  empty cels.
* `point4frames.aseprite`: Indexed, 4 frames, 2 layers, same cel
  content but different positions, can be used to test
  `-merge-duplicates` to check if all cels go to the same sprite sheet
  position.
* `point2frames.aseprite`: Indexed, 2 frames, 1 layer, same cel
  content as in `point4frames.aseprite`. Useful to test if
  `"sourceSize"` is different when two cels from different sprites are
  merged in the same texture atlas.
* `2f-index-3x3.aseprite`: Indexed, 2 frames, 1 layer, mask color set
  to index 21.
* `4f-index-4x4.aseprite`: Indexed, 4 frames, 1 layer, mask color set
  to index 0.
* `file-tests-props.aseprite`: Indexed, 64x64, 6 frames, 4 layers (one
   of them is a tilemap), 13 cels, 1 tag, pre-defined user data
   properties of all kinds in several sprite elements.
* `slices.aseprite`: Indexed, 4x4, background layer, 2 slices.
* `slices-moving.aseprite`: Indexed, 4x4, 1 linked cel in 4 frames,
  background layer, 1 slice with 4 keyframes (each keyframe with a
  different position/size).
* `2x2tilemap2x2tile.aseprite`: RGB, 6x6, 2x2 tilemap layer, 5 tiles tileset,
  2x2 tile size, 1 frame.
* `2x3tilemap-indexed.aseprite`: Indexed, 4x5, 1 tilemap layer,
  5 tiles tileset, 2x3 tile size, 3 frames
* `3x2tilemap-grayscale.aseprite`: Grayscale, 6x6, 2 layers (one of them
  is a tilemap), 4 cels, 4 tiles tileset, 3x2 tile size, 4 frames
