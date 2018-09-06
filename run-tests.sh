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
$ASEPRITE -b --script scripts/console_assert.lua >tmp 2>tmp_err
! grep -q "this should be in the output" tmp && fail "print() text not found in output"
! grep -q "assertion failed" tmp && fail "assert() text not found in output"
grep -q "this should not be in the output" tmp && fail "text that shouldn't be in the output is"

if [[ "$(uname)" =~ "MINGW32" ]] || [[ "$(uname)" =~ "MSYS_NT-10.0" ]] ; then
    echo Ignore console tests on Windows
else
    $ASEPRITE -b --script scripts/console_print.lua >tmp 2>tmp_err
    cat >tmp_expected <<EOF
hello world
1	2	3
EOF
    ! diff -u tmp tmp_expected && fail
fi

echo ----------------------------------------------------------------------
echo "Testing scripts..."

result=0
for script in scripts/*.lua ; do
    [[ $script =~ console ]] && continue

    echo "Running $script"
    if ! $ASEPRITE -b --script $script >tmp 2>tmp_err ; then
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
