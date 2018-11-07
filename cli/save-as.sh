#! /bin/bash
# Copyright (C) 2018 Igara Studio S.A.

function list_files() {
    oldwd=$(pwd)
    cd $1 && ls -1 *.*
    cd $oldwd
}

# --save-as

d=$t/save-as
$ASEPRITE -b sprites/1empty3.aseprite --save-as "$d/image00.png"
expect "image00.png
image01.png
image02.png" "list_files $d"

# --ignore-empty --save-as

d=$t/save-as-ignore-empty
$ASEPRITE -b sprites/1empty3.aseprite --ignore-empty --save-as $d/image00.png
expect "image00.png
image02.png" "list_files $d"
