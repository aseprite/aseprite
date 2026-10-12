# Aseprite Undo History File Format (.aseprite-undo) Specifications

> Warning: This is a work-in-progress and this format might change in
> a near future.

This file specifies the format of `.aseprite-undo` files or the "Undo
chunk" in [`.aseprite` files](ase-file-spec.md).

## OP Codes

    XXyy : XX = Object, yy = opcode

Objects:

    CD = Cel Data
    CE = Cel
    FR = Frame
    IM = Image
    LA = Layer
    PA = Palette
    SE = Selection / Mask
    SL = Slice
    SP = Sprite
    TG = Tag
    TI = Tile
    TM = Tilemap(s)
    TS = Tileset
    TX = Transaction
    UD = User Data

Actions:

    ac = assign color profile
    ad = add
    al = set opacity / alpha
    bg = convert to background
    bi = base index
    bm = set blend mode
    bo = set bounds
    cb = configure background
    cc = convert color profile
    cl = clear
    cm = set color mode
    cp = assign / set / copy
    cr = clear rectangle
    da = set user data
    de = delete / deselect / remove / subtract
    di = set anidir
    du = set duration
    fp = flip
    fr = from
    fs = set flags
    gr = set grid
    im = set image
    la = convert to normal layer
    mp = remap
    mv = move
    na = rename / set name
    pa = patch / union
    pr = set property
    ps = set properties
    pu = set tile management plugin
    px = set pixel ratio
    ra = set range
    rc = copy rect
    re = replace / reselect
    rg = copy region
    rt = repeat
    sf = set frame(s)
    sk = set key
    sz = set size
    tc = transparent color
    to = configure to / convert to
    ts = set tileset
    un = unlink
    xy = set xy position
    zi = set z-index

## Text Format

The text format of `.aseprite-undo` is for debugging/development
purposes only. The binary format is the preferred one for end-users.

The first line specifies IDs of every object in the `.aseprite` file
in the same order as they appear in that file:

    SP sprite_id LA root_layer_group_id [OBJECT ID]...

So this order must match the `.aseprite` content to assign an ID to
every object. This ID is used on each step/undo transaction to
identify each object.

Then each undoable action/transaction in the file is represented by
each line in the following format:

    TX [...]

And you will notice an `*` indicating the current state of the file:

    *TX [...]

This means that the current sprite state (the `.aseprite` file) is
just before executing that specific transaction `TX` (that `TX` would
be the first "redo" action in the history).

TODO

## Binary Format

This can be stored inside an `.aseprite-undo` file or in an "Undo
chunk" inside [`.aseprite` files](ase-file-spec.md).

Header:

    WORD        Serialized
    WORD        Number of labels
    DWORD       Number of objects
    DWORD       Number of transaction
    BYTE[54]    Reserved

Array of labels, objects, and transaction:

    + For each label ("array of labels")
      STRING    Label

    + For each object
      WORD      Object Type (e.g. "SP", or "LA")
      DWORD     Object ID

    + For each transaction
      WORD      Fixed code for transactions 'TX'
      WORD      Label reference (index to the "array of labels")
      DWORD     Transaction length (in bytes)
      DWORD     Number of ops
      + For each op
        DWORD   OP code in the "XXyy" format (Object + operation)
        DWORD   Operation data length
        BYTE[]  Operation data

Depending on the specific operation, the data will have a completelly
different format.
