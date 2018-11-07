#! /bin/sh
# Copyright (C) 2018 Igara Studio S.A.
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

function expect() {
    if [[ $1 != $($2) ]] ; then
	echo "FAILED: $2"
	echo "EXPECTED: $1"
	echo "RESULT: $($2)"
	exit 1
    fi
}

echo ----------------------------------------------------------------------
echo $ASEPRITE --version
$ASEPRITE --version

echo ----------------------------------------------------------------------
echo Temp dir
t=$(mktemp -d)
echo $t

echo ----------------------------------------------------------------------
echo "Testing console..."
$ASEPRITE -b --script scripts/console_assert.lua >$t/tmp 2>$t/tmp_err
! grep -q "this should be in the output" $t/tmp && fail "print() text not found in output"
! grep -q "assertion failed" $t/tmp && fail "assert() text not found in output"
grep -q "this should not be in the output" $t/tmp && fail "text that shouldn't be in the output is"

if [[ "$(uname)" =~ "MINGW32" ]] || [[ "$(uname)" =~ "MSYS_NT-10.0" ]] ; then
    echo Ignore console tests on Windows
else
    $ASEPRITE -b --script scripts/console_print.lua >$t/tmp 2>$t/tmp_err
    cat >$t/tmp_expected <<EOF
hello world
1	2	3
EOF
    ! diff -u $t/tmp $t/tmp_expected && fail
fi

echo ----------------------------------------------------------------------
echo "Testing scripts..."

result=0
for script in scripts/*.lua ; do
    [[ $script =~ console ]] && continue
    echo "Running $script"
    if ! $ASEPRITE -b --script $script >$t/tmp 2>$t/tmp_err ; then
        echo FAILED
        echo STDOUT && cat $t/tmp
        echo STDERR && cat $t/tmp_err
        result=1
    fi
done

echo ----------------------------------------------------------------------
echo "Testing CLI..."

result=0
for script in cli/*.sh ; do
    echo "Running $script"
    source $script
done

echo ----------------------------------------------------------------------
echo Done
echo ----------------------------------------------------------------------

exit $result
