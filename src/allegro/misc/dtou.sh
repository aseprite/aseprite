#!/bin/sh
#
# Convert CR/LF line endings to LF line endings, preserving timestamps and
# permissions on the file.
#

with_unix_tools() {
   for file in "$@"
   do
      echo "$file"

      tmpfile=`dirname "$file"`/__dtou_tmp.$RANDOM || exit 1
      trap 'rm -f "$tmpfile"' 1 2 3 13 15

      # We go through a slightly convoluted sequence of commands in order to
      # preserve both the timestamp and permissions on the file.
      {
         tr -d '\015' < "$file" > "$tmpfile" &&
         touch -r "$file" "$tmpfile" &&
         cat "$tmpfile" > "$file" &&
         touch -r "$tmpfile" "$file" &&
         rm -f "$tmpfile"
      } || exit 1
   done
}

with_cygwin() {
   for file in "$@"
   do
      dos2unix $file || exit 1
   done
}

if test -z "$1"
then
   echo "$0 filename"
   exit
fi

if test "$ALLEGRO_USE_CYGWIN" = "1"
then
   with_cygwin "$@"
else
   with_unix_tools "$@"
fi

# vi: sts=3 sw=3 et
