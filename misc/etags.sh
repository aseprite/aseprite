#! /bin/sh

find data/scripts src third_party \
   \( -name '*.[ch]' -o \
      -name '*.lua' \) -print | \
    sed -e "/_old/D" | \
    etags -
