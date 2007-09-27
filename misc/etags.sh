#! /bin/sh

find data/scripts jinete src third_party \
   \( -name '*.[ch]' -o \
      -name '*.lua' \) -print | \
    sed -e "/_old/D" | \
    etags -
