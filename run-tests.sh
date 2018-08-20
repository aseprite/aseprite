#! /bin/sh
# Copyright (C) 2018 David Capello

if [[ "$ASEPRITE" == "" ]]; then
    echo ASEPRITE env var must be pointing to the Aseprite executable
    exit 1
fi

function fail() {
    echo FAIL
    echo $BASH_SOURCE:$BASH_LINENO: error: $*
    exit 1
}

echo ----------------------------------------------------------------------
echo $ASEPRITE --version
$ASEPRITE --version

echo ----------------------------------------------------------------------
echo "Testing console..."
$ASEPRITE -b --script scripts/console_assert.js >tmp 2>tmp_err
! grep -q Error tmp && fail
! grep -q "this should be in the output" tmp && fail
grep -q "this should not be in the output" tmp && fail

$ASEPRITE -b --script scripts/console_log.js >tmp 2>tmp_err
cat >tmp_expected <<EOF
hello world
1 2 3
EOF
! diff -u tmp tmp_expected && fail

echo ----------------------------------------------------------------------
echo "Testing scripts..."
result=0
for jsfile in scripts/*.js ; do
    [[ $jsfile =~ console ]] && continue

    echo "Running $jsfile"
    if ! $ASEPRITE -b --script $jsfile >tmp 2>tmp_err ; then
        echo FAILED
        echo STDOUT && cat tmp
        echo STDERR && cat tmp_err
        result=1
    fi
done

echo ----------------------------------------------------------------------
echo Done
echo ----------------------------------------------------------------------

exit $result
