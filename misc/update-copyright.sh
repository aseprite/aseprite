#! /bin/bash
#
# Usage:
#
#   misc/update-copyright.sh [--apply]
#

for f in $(git status --porcelain=1 | grep -e "^M " | cut -d' ' -f3) ; do
    tmpfile=$(mktemp)
    echo "$f"
    if cat "$f" | grep Copyright | grep "Igara Studio" >/dev/null ; then
        cat "$f" | sed -e 's/\(^.*Copyright ... 20..\)\(-20..\)\?\( \+Igara Studio S.A.\)/\1-present\3/' > "$tmpfile"
    elif cat "$f" | grep Copyright | grep "David Capello" >/dev/null ; then
        year=$(date +%Y)
        cat "$f" | sed -e "s/\(^\(.\+\) Copyright \(([cC])\) \([^ ]*\)\([ ]\+\)David Capello\)/\2 Copyright \3 $year-present\5Igara Studio S.A.\n\1/" > "$tmpfile"
    else
        echo "  Copyright not found"
        continue
    fi
    if ! cmp "$f" "$tmpfile" >/dev/null ; then
        head "$tmpfile" -n 4 | sed 's/^/  /'
        if [[ "$1" == "--apply" ]] ; then
            cat "$tmpfile" > "$f"
            echo "[PATCHED] $f"
        fi
    fi
done
