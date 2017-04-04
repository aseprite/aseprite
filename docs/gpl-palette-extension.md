GIMP Palette File Format Extension
====================================================

Aseprite can load/save GIMP Palettes (`.gpl` files) extended with
alpha information using the following format:

```
GIMP Palette
Channels: RGBA
#
  0   0   0   0 Transparent
254  91  89 255 Red
247 165  71 255 Orange
243 206  82 255 Yellow
106 205  91 255 Green
 87 185 242 255 Blue
209 134 223 255 Purple
165 165 167 255 Gray
```

You must specify `Channels: RGBA` in the header and then each entry
must contain an extra alpha value. There are no plans to provide a
different value for `Channels` properties.

Note that this is an Aseprite extension, GIMP does not support
palettes with alpha values.
