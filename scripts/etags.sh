#! /bin/sh

find src third_party \
   \( -name '*.[ch]' -o -name '*.cpp' \) -print | \
    sed -e "/_old/D" | \
    etags -

ebrowse $(find src \( -name '*.h' -o -name '*.cpp' \) -print)
