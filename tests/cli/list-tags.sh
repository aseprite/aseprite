#! /bin/bash
# Copyright (C) 2018 Igara Studio S.A.

expect "a
b" "$ASEPRITE -b --list-tags sprites/1empty3.aseprite"
