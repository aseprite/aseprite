#! /bin/bash
# Copyright (C) 2018 Igara Studio S.A.

expect "bg
fg" "$ASEPRITE -b --list-layers sprites/1empty3.aseprite"

expect "a
b
c
d" "$ASEPRITE -b --list-layers sprites/abcd.aseprite"
