#! /bin/bash
# Copyright (C) 2018 Igara Studio S.A.

if ! $ASEPRITE --help | grep "\\-\\-help" > /dev/null ; then
    echo "FAILED: --help doesn't include usage information"
    exit 1
fi
