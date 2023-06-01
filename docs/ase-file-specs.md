# Aseprite File Format (.ase/.aseprite) Specifications

1. [References](#references)
2. [Introduction](#introduction)
3. [Header](#header)
4. [Frames](#frames)
5. [Chunk Types](#chunk-types)
6. [Notes](#notes)
7. [File Format Changes](#file-format-changes)

## References

ASE files use Intel (little-endian) byte order.

* `BYTE`: An 8-bit unsigned integer value
* `WORD`: A 16-bit unsigned integer value
* `SHORT`: A 16-bit signed integer value
* `DWORD`: A 32-bit unsigned integer value
* `LONG`: A 32-bit signed integer value
* `FIXED`: A 32-bit fixed point (16.16) value
* `FLOAT`: A 32-bit single-precision value
* `DOUBLE`: A 64-bit double-precision value
* `QWORD`: A 64-bit unsigned integer value
* `LONG64`: A 64-bit signed integer value
* `BYTE[n]`: "n" bytes.
* `STRING`:
  - `WORD`: string length (number of bytes)
  - `BYTE[length]`: characters (in UTF-8)
  The `'\0'` character is not included.
* `POINT`:
  - `LONG`: X coordinate value
  - `LONG`: Y coordinate value
* `SIZE`:
  - `LONG`: Width value
  - `LONG`: Height value
* `RECT`:
  - `POINT`: Origin coordinates
  - `SIZE`: Rectangle size
* `PIXEL`: One pixel, depending on the image pixel format:
  - **RGBA**: `BYTE[4]`, each pixel have 4 bytes in this order Red, Green, Blue, Alpha.
  - **Grayscale**: `BYTE[2]`, each pixel have 2 bytes in the order Value, Alpha.
  - **Indexed**: `BYTE`, each pixel uses 1 byte (the index).
* `TILE`: **Tilemaps**: Each tile can be a 8-bit (`BYTE`), 16-bit
  (`WORD`), or 32-bit (`DWORD`) value and there are masks related to
  the meaning of each bit.
* `UUID`: A Universally Unique Identifier stored as `BYTE[16]`.

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

The chunk size includes the DWORD of the size itself, and the WORD of
the chunk type, so a chunk size must be equal or greater than 6 bytes
at least.

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
                  2 = Tilemap
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
    + If layer type = 2
      DWORD     Tileset index

### Cel Chunk (0x2005)

This chunk determine where to put a cel in the specified layer/frame.

    WORD        Layer index (see NOTE.2)
    SHORT       X position
    SHORT       Y position
    BYTE        Opacity level
    WORD        Cel Type
                0 - Raw Image Data (unused, compressed image is preferred)
                1 - Linked Cel
                2 - Compressed Image
                3 - Compressed Tilemap
    SHORT       Z-Index (see NOTE.5)
                0 = default layer ordering
                +N = show this cel N layers later
                -N = show this cel N layers back
    BYTE[5]     For future (set to zero)
    + For cel type = 0 (Raw Image Data)
      WORD      Width in pixels
      WORD      Height in pixels
      PIXEL[]   Raw pixel data: row by row from top to bottom,
                for each scanline read pixels from left to right.
    + For cel type = 1 (Linked Cel)
      WORD      Frame position to link with
    + For cel type = 2 (Compressed Image)
      WORD      Width in pixels
      WORD      Height in pixels
      PIXEL[]   "Raw Cel" data compressed with ZLIB method (see NOTE.3)
    + For cel type = 3 (Compressed Tilemap)
      WORD      Width in number of tiles
      WORD      Height in number of tiles
      WORD      Bits per tile (at the moment it's always 32-bit per tile)
      DWORD     Bitmask for tile ID (e.g. 0x1fffffff for 32-bit tiles)
      DWORD     Bitmask for X flip
      DWORD     Bitmask for Y flip
      DWORD     Bitmask for 90CW rotation
      BYTE[10]  Reserved
      TILE[]    Row by row, from top to bottom tile by tile
                compressed with ZLIB method (see NOTE.3)

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
    BYTE[8]     Reserved (set to zero)
    + If type = ICC:
      DWORD     ICC profile data length
      BYTE[]    ICC profile data. More info: http://www.color.org/ICC1V42.pdf

### External Files Chunk (0x2008)

A list of external files linked with this file can be found in the first frame. It might be used to
reference external palettes, tilesets, or extensions that make use of extended properties.

    DWORD       Number of entries
    BYTE[8]     Reserved (set to zero)
    + For each entry
      DWORD     Entry ID (this ID is referenced by tilesets, palettes, or extended properties)
      BYTE      Type
                  0 - External palette
                  1 - External tileset
                  2 - Extension name for properties
                  3 - Extension name for tile management (can exist one per sprite)
      BYTE[7]   Reserved (set to zero)
      STRING    External file name or extension ID (see NOTE.4)

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

After the tags chunk, you can write one user data chunk for each tag.
E.g. if there are 10 tags, you can then write 10 user data chunks one
for each tag.

    WORD        Number of tags
    BYTE[8]     For future (set to zero)
    + For each tag
      WORD      From frame
      WORD      To frame
      BYTE      Loop animation direction
                  0 = Forward
                  1 = Reverse
                  2 = Ping-pong
                  3 = Ping-pong Reverse
      WORD      Repeat N times. Play this animation section N times:
                  0 = Doesn't specify (plays infinite in UI, once on export,
                      for ping-pong it plays once in each direction)
                  1 = Plays once (for ping-pong, it plays just in one direction)
                  2 = Plays twice (for ping-pong, it plays once in one direction,
                      and once in reverse)
                  n = Plays N times
      BYTE[6]   For future (set to zero)
      BYTE[3]   RGB values of the tag color
                  Deprecated, used only for backward compatibility with Aseprite v1.2.x
                  The color of the tag is the one in the user data field following
                  the tags chunk
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

Specifies the user data (color/text/properties) to be associated with
the last read chunk/object. E.g. If the last chunk we've read is a
layer and then this chunk appears, this user data belongs to that
layer, if we've read a cel, it belongs to that cel, etc. There are
some special cases:

1. After a Tags chunk, there will be several user data chunks, one for
   each tag, you should associate the user data in the same order as
   the tags are in the Tags chunk.
2. After the Tileset chunk, it could be followed by a user data chunk
   (empty or not) and then all the user data chunks of the tiles
   ordered by tile index, or it could be followed by none user data
   chunk (if the file was created in an older Aseprite version of if
   no tile has user data).
3. In Aseprite v1.3 a sprite has associated user data, to consider
   this case there is an User Data Chunk at the first frame after the
   Palette Chunk.

The data of this chunk is as follows:

    DWORD       Flags
                  1 = Has text
                  2 = Has color
                  4 = Has properties
    + If flags have bit 1
      STRING    Text
    + If flags have bit 2
      BYTE      Color Red (0-255)
      BYTE      Color Green (0-255)
      BYTE      Color Blue (0-255)
      BYTE      Color Alpha (0-255)
    + If flags have bit 4
      DWORD     Size in bytes of all properties maps stored in this chunk
                The size includes the this field and the number of property maps
                (so it will be a value greater or equal to 8 bytes).
      DWORD     Number of properties maps
      + For each properties map:
        DWORD     Properties maps key
                  == 0 means user properties
                  != 0 means an extension Entry ID (see External Files Chunk))
        DWORD     Number of properties
        + For each property:
          STRING    Name
          WORD      Type
          + If type==0x0001 (bool)
            BYTE    == 0 means FALSE
                    != 0 means TRUE
          + If type==0x0002 (int8)
            BYTE
          + If type==0x0003 (uint8)
            BYTE
          + If type==0x0004 (int16)
            SHORT
          + If type==0x0005 (uint16)
            WORD
          + If type==0x0006 (int32)
            LONG
          + If type==0x0007 (uint32)
            DWORD
          + If type==0x0008 (int64)
            LONG64
          + If type==0x0009 (uint64)
            QWORD
          + If type==0x000A
            FIXED
          + If type==0x000B
            FLOAT
          + If type==0x000C
            DOUBLE
          + If type==0x000D
            STRING
          + If type==0x000E
            POINT
          + If type==0x000F
            SIZE
          + If type==0x0010
            RECT
          + If type==0x0011 (vector)
            DWORD     Number of elements
            WORD      Element's type.
            + If Element's type == 0 (all elements are not of the same type)
              For each element:
                WORD      Element's type
                BYTE[]    Element's value. Structure depends on the
                          element's type
            + Else (all elements are of the same type)
              For each element:
                BYTE[]    Element's value. Structure depends on the
                          element's type
          + If type==0x0012 (nested properties map)
            DWORD     Number of properties
            BYTE[]    Nested properties data
                      Structure is the same as indicated in this loop
          + If type==0x0013
            UUID

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

### Tileset Chunk (0x2023)

    DWORD       Tileset ID
    DWORD       Tileset flags
                  1 - Include link to external file
                  2 - Include tiles inside this file
                  4 - Tilemaps using this tileset use tile ID=0 as empty tile
                      (this is the new format). In rare cases this bit is off,
                      and the empty tile will be equal to 0xffffffff (used in
                      internal versions of Aseprite)
    DWORD       Number of tiles
    WORD        Tile Width
    WORD        Tile Height
    SHORT       Base Index: Number to show in the screen from the tile with
                index 1 and so on (by default this is field is 1, so the data
                that is displayed is equivalent to the data in memory). But it
                can be 0 to display zero-based indexing (this field isn't used
                for the representation of the data in the file, it's just for
                UI purposes).
    BYTE[14]    Reserved
    STRING      Name of the tileset
    + If flag 1 is set
      DWORD     ID of the external file. This ID is one entry
                of the the External Files Chunk.
      DWORD     Tileset ID in the external file
    + If flag 2 is set
      DWORD     Compressed data length
      PIXEL[]   Compressed Tileset image (see NOTE.3):
                  (Tile Width) x (Tile Height x Number of Tiles)

## Notes

### NOTE.1

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

### NOTE.2

The layer index is a number to identify a layer in the sprite. Layers
are numbered in the same order as Layer Chunks (0x2004) appear in the
file, for example:

    Layer name and hierarchy      Layer index
    -----------------------------------------------
    - Background                  0
      `- Layer1                   1
    - Foreground                  2
      |- My set1                  3
      |  `- Layer2                4
      `- Layer3                   5

It means that in the file you will find the `Background` layer chunk
first, then the `Layer1` layer chunk, etc.

### NOTE.3

**Uncompressed Image**: Uncompressed ("raw") images inside `.aseprite`
files are saved row by row from top to bottom, and for each
row/scanline, pixels are from left to right. Each pixel is a `PIXEL`
(or a `TILE` in the case of tilemaps) as defined in the
[References](#references) section (so the number and order of bytes
depends on the color mode of the image/sprite, or the tile
format). Generally you'll not find uncompressed images in `.aseprite`
files (only in very old `.aseprite` files).

**Compressed Image**: When an image is compressed (the regular case
that you will find in `.aseprite` files), the data is a stream of
bytes in exactly the same *"Uncompressed Image"* format as described
above, but compressed using the ZLIB method. Details about the ZLIB
and DEFLATE compression methods can be found here:

* https://www.ietf.org/rfc/rfc1950
* https://www.ietf.org/rfc/rfc1951
* Some extra notes that might help you to decode the data:
  http://george.chiramattel.com/blog/2007/09/deflatestream-block-length-does-not-match.html

### NOTE.4

The extension ID must be a string like `publisher/ExtensionName`, for
example, the [Aseprite Attachment System](https://github.com/aseprite/Attachment-System)
uses `aseprite/Attachment-System`.

This string will be used in a future to automatically link to the
extension URL in the [Aseprite Store](https://github.com/aseprite/aseprite/issues/1928).

### NOTE.5

In case that you read and render an `.aseprite` file in your game
engine/software, you are going to need to process the z-index field
for each cel with a specific algorithm. This is a possible C++ code
about how to order layers for a specific frame (the `zIndex` must be
set depending on the active frame/cel):

```c++
struct Layer {
  int layerIndex; // See the "layer index" in NOTE.2
  int zIndex;     // The z-index value for a specific cel in this layer/frame

  int order() const {
    return layerIndex + zIndex;
  }

  // Function to order with std::sort() by operator<(),
  // which establish the render order from back to front.
  bool operator<(const Layer& b) const {
    return (order() < b.order()) ||
           (order() == b.order() && (zIndex < b.zIndex));
  }
};
```

Basically we first compare `layerIndex + zIndex` of each cel, and then
if this value is the same, we compare the specific `zIndex` value to
disambiguate some scenarios. An example of this implementation can be
found in the [RenderPlan code](https://github.com/aseprite/aseprite/blob/8e91d22b704d6d1e95e1482544318cee9f166c4d/src/doc/render_plan.cpp#L77).

## File Format Changes

1. The first change from the first release of the new .ase format,
   is the new frame duration field. This is because now each frame
   can have different duration.

   How to read both formats (old and new one)?  You should set all
   frames durations to the "speed" field read from the main ASE
   header.  Then, if you found a frame with the frame-duration
   field > 0, you should update the duration of the frame with
   that value.
