# Aseprite File Format (.ase/.aseprite) Specifications

1. [References](#references)
2. [Introduction](#introduction)
3. [Header](#header)
4. [Frames](#frames)
5. [Chunk Types](#chunk-types)
6. [File Format Changes](#file-format-changes)

## References

ASE files use Intel (little-endian) byte order.

* `BYTE`: An 8-bit unsigned integer value
* `WORD`: A 16-bit unsigned integer value
* `SHORT`: A 16-bit signed integer value
* `DWORD`: A 32-bit unsigned integer value
* `LONG`: A 32-bit signed integer value
* `FIXED`: A 32-bit fixed point (16.16) value
* `BYTE[n]`: "n" bytes.
* `STRING`:
  - `WORD`: string length (number of bytes)
  - `BYTE[length]`: characters (in UTF-8)
  The `'\0'` character is not included.
* `PIXEL`: One pixel, depending on the image pixel format:
  - **RGBA**: `BYTE[4]`, each pixel have 4 bytes in this order Red, Green, Blue, Alpha.
  - **Grayscale**: `BYTE[2]`, each pixel have 2 bytes in the order Value, Alpha.
  - **Indexed**: `BYTE`, Each pixel uses 1 byte (the index).

## Introduction

The format is much like FLI/FLC files, but with different magic number
and differents chunks.  Also, the color depth can be 8, 16 or 32 for
Indexed, Grayscale and RGB respectively, and images are compressed
images with zlib.  Color palettes are in FLI color chunks (it could be
type=11 or type=4). For color depths more than 8bpp, palettes are
optional.

To read the sprite:

* Read the [ASE header](#header)
* For each frame do (how many frames? the ASE header has that information):
  + Read the [frame header](#frames)
  + For each chunk in this frame (how many chunks? the frame header has that information)
    - Read the chunk (it should be layer information, a cel or a palette)

## Header

A 128-byte header (same as FLC/FLI header, but with other magic number):

    DWORD       File size
    WORD        Magic number (0xA5E0)
    WORD        Frames
    WORD        Width in pixels
    WORD        Height in pixels
    WORD        Color depth (bits per pixel)
                  32 bpp = RGBA
                  16 bpp = Grayscale
                  8 bpp = Indexed
    DWORD       Flags:
                  1 = Layer opacity has valid value
    WORD        Speed (milliseconds between frame, like in FLC files)
                DEPRECATED: You should use the frame duration field
                from each frame header
    DWORD       Set be 0
    DWORD       Set be 0
    BYTE        Palette entry (index) which represent transparent color
                in all non-background layers (only for Indexed sprites).
    BYTE[3]     Ignore these bytes
    WORD        Number of colors (0 means 256 for old sprites)
    BYTE        Pixel width (pixel ratio is "pixel width/pixel height").
                If this or pixel height field is zero, pixel ratio is 1:1
    BYTE        Pixel height
    SHORT       X position of the grid
    SHORT       Y position of the grid
    WORD        Grid width (zero if there is no grid, grid size
                is 16x16 on Aseprite by default)
    WORD        Grid height (zero if there is no grid)
    BYTE[84]    For future (set to zero)

## Frames

After the header come the "frames" data. Each frame has this little
header of 16 bytes:

    DWORD       Bytes in this frame
    WORD        Magic number (always 0xF1FA)
    WORD        Old field which specifies the number of "chunks"
                in this frame. If this value is 0xFFFF, we might
                have more chunks to read in this frame
                (so we have to use the new field)
    WORD        Frame duration (in milliseconds)
    BYTE[2]     For future (set to zero)
    DWORD       New field which specifies the number of "chunks"
                in this frame (if this is 0, use the old field)

Then each chunk format is:

    DWORD       Chunk size
    WORD        Chunk type
    BYTE[]      Chunk data

## Chunk Types

### Old palette chunk (0x0004)

Ignore this chunk if you find the new palette chunk (0x2019) Aseprite
v1.1 saves both chunks 0x0004 and 0x2019 just for backward
compatibility.

    WORD        Number of packets
    + For each packet
      BYTE      Number of palette entries to skip from the last packet (start from 0)
      BYTE      Number of colors in the packet (0 means 256)
      + For each color in the packet
        BYTE    Red (0-255)
        BYTE    Green (0-255)
        BYTE    Blue (0-255)

### Old palette chunk (0x0011)

Ignore this chunk if you find the new palette chunk (0x2019)

    WORD        Number of packets
    + For each packet
      BYTE      Number of palette entries to skip from the last packet (start from 0)
      BYTE      Number of colors in the packet (0 means 256)
      + For each color in the packet
        BYTE    Red (0-63)
        BYTE    Green (0-63)
        BYTE    Blue (0-63)

### Layer Chunk (0x2004)

  In the first frame should be a set of layer chunks to determine the
  entire layers layout:

    WORD        Flags:
                  1 = Visible
                  2 = Editable
                  4 = Lock movement
                  8 = Background
                  16 = Prefer linked cels
                  32 = The layer group should be displayed collapsed
                  64 = The layer is a reference layer
    WORD        Layer type
                  0 = Normal (image) layer
                  1 = Group
    WORD        Layer child level (see NOTE.1)
    WORD        Default layer width in pixels (ignored)
    WORD        Default layer height in pixels (ignored)
    WORD        Blend mode (always 0 for layer set)
                  Normal         = 0
                  Multiply       = 1
                  Screen         = 2
                  Overlay        = 3
                  Darken         = 4
                  Lighten        = 5
                  Color Dodge    = 6
                  Color Burn     = 7
                  Hard Light     = 8
                  Soft Light     = 9
                  Difference     = 10
                  Exclusion      = 11
                  Hue            = 12
                  Saturation     = 13
                  Color          = 14
                  Luminosity     = 15
                  Addition       = 16
                  Subtract       = 17
                  Divide         = 18
    BYTE        Opacity
                  Note: valid only if file header flags field has bit 1 set
    BYTE[3]     For future (set to zero)
    STRING      Layer name

### Cel Chunk (0x2005)

  This chunk determine where to put a cel in the specified
  layer/frame.

    WORD        Layer index (see NOTE.2)
    SHORT       X position
    SHORT       Y position
    BYTE        Opacity level
    WORD        Cel type
    BYTE[7]     For future (set to zero)
    + For cel type = 0 (Raw Cel)
      WORD      Width in pixels
      WORD      Height in pixels
      PIXEL[]   Raw pixel data: row by row from top to bottom,
                for each scanline read pixels from left to right.
    + For cel type = 1 (Linked Cel)
      WORD      Frame position to link with
    + For cel type = 2 (Compressed Image)
      WORD      Width in pixels
      WORD      Height in pixels
      BYTE[]    "Raw Cel" data compressed with ZLIB method

Details about the ZLIB and DEFLATE compression methods:

* https://www.ietf.org/rfc/rfc1950
* https://www.ietf.org/rfc/rfc1951
* Some extra notes that might help you to decode the data:
  http://george.chiramattel.com/blog/2007/09/deflatestream-block-length-does-not-match.html

### Cel Extra Chunk (0x2006)

Adds extra information to the latest read cel.

    DWORD       Flags (set to zero)
                  1 = Precise bounds are set
    FIXED       Precise X position
    FIXED       Precise Y position
    FIXED       Width of the cel in the sprite (scaled in real-time)
    FIXED       Height of the cel in the sprite
    BYTE[16]    For future use (set to zero)

### Color Profile Chunk (0x2007)

Color profile for RGB or grayscale values.

    WORD        Type
                  0 - no color profile (as in old .aseprite files)
                  1 - use sRGB
                  2 - use the embedded ICC profile
    WORD        Flags
                  1 - use special fixed gamma
    FIXED       Fixed gamma (1.0 = linear)
                Note: The gamma in sRGB is 2.2 in overall but it doesn't use
                this fixed gamma, because sRGB uses different gamma sections
                (linear and non-linear). If sRGB is specified with a fixed
                gamma = 1.0, it means that this is Linear sRGB.
    BYTE[8]     Reserved (set to zero]
    + If type = ICC:
      DWORD     ICC profile data length
      BYTE[]    ICC profile data. More info: http://www.color.org/ICC1V42.pdf

### Mask Chunk (0x2016) DEPRECATED

    SHORT       X position
    SHORT       Y position
    WORD        Width
    WORD        Height
    BYTE[8]     For future (set to zero)
    STRING      Mask name
    BYTE[]      Bit map data (size = height*((width+7)/8))
                Each byte contains 8 pixels (the leftmost pixels are
                packed into the high order bits)

### Path Chunk (0x2017)

Never used.

### Tags Chunk (0x2018)

    WORD        Number of tags
    BYTE[8]     For future (set to zero)
    + For each tag
      WORD      From frame
      WORD      To frame
      BYTE      Loop animation direction
                  0 = Forward
                  1 = Reverse
                  2 = Ping-pong
      BYTE[8]   For future (set to zero)
      BYTE[3]   RGB values of the tag color
      BYTE      Extra byte (zero)
      STRING    Tag name

### Palette Chunk (0x2019)

    DWORD       New palette size (total number of entries)
    DWORD       First color index to change
    DWORD       Last color index to change
    BYTE[8]     For future (set to zero)
    + For each palette entry in [from,to] range (to-from+1 entries)
      WORD      Entry flags:
                  1 = Has name
      BYTE      Red (0-255)
      BYTE      Green (0-255)
      BYTE      Blue (0-255)
      BYTE      Alpha (0-255)
      + If has name bit in entry flags
        STRING  Color name

### User Data Chunk (0x2020)

Insert this user data in the last read chunk.  E.g. If we've read a
layer, this user data belongs to that layer, if we've read a cel, it
belongs to that cel, etc.

    DWORD       Flags
                  1 = Has text
                  2 = Has color
    + If flags have bit 1
      STRING    Text
    + If flags have bit 2
      BYTE      Color Red (0-255)
      BYTE      Color Green (0-255)
      BYTE      Color Blue (0-255)
      BYTE      Color Alpha (0-255)

### Slice Chunk (0x2022)

    DWORD       Number of "slice keys"
    DWORD       Flags
                  1 = It's a 9-patches slice
                  2 = Has pivot information
    DWORD       Reserved
    STRING      Name
    + For each slice key
      DWORD     Frame number (this slice is valid from
                this frame to the end of the animation)
      LONG      Slice X origin coordinate in the sprite
      LONG      Slice Y origin coordinate in the sprite
      DWORD     Slice width (can be 0 if this slice hidden in the
                animation from the given frame)
      DWORD     Slice height
      + If flags have bit 1
        LONG    Center X position (relative to slice bounds)
        LONG    Center Y position
        DWORD   Center width
        DWORD   Center height
      + If flags have bit 2
        LONG    Pivot X position (relative to the slice origin)
        LONG    Pivot Y position (relative to the slice origin)

### Notes

#### NOTE.1

The child level is used to show the relationship of this layer with
the last one read, for example:

    Layer name and hierarchy      Child Level
    -----------------------------------------------
    - Background                  0
      `- Layer1                   1
    - Foreground                  0
      |- My set1                  1
      |  `- Layer2                2
      `- Layer3                   1

#### NOTE.2

The layer index is a number to identify any layer in the sprite, for
example:

    Layer name and hierarchy      Layer index
    -----------------------------------------------
    - Background                  0
      `- Layer1                   1
    - Foreground                  2
      |- My set1                  3
      |  `- Layer2                4
      `- Layer3                   5

## File Format Changes

1. The first change from the first release of the new .ase format,
   is the new frame duration field. This is because now each frame
   can have different duration.

   How to read both formats (old and new one)?  You should set all
   frames durations to the "speed" field read from the main ASE
   header.  Then, if you found a frame with the frame-duration
   field > 0, you should update the duration of the frame with
   that value.
