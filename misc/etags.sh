#! /bin/sh

find src third_party \
   \( -name '*.[ch]' -o -name '*.cpp' \) -print | \
    sed -e "/_old/D" | \
    etags -
