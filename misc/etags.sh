#! /bin/sh

find src third_party \
   \( -name '*.[ch]' \) -print | \
    sed -e "/_old/D" | \
    etags -
