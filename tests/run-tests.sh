#! /bin/bash
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
    if [[ $1 != "$($2 | tr -d "\r")" ]] ; then
        echo "FAILED: $2"
        echo "EXPECTED: $1"
        echo "RESULT: $($2)"
        exit 1
    fi
}

# General information
echo ----------------------------------------------------------------------
echo $ASEPRITE --version
$ASEPRITE --version

filter="$*"
if [[ "$filter" != "" ]]; then
    echo Filter: $filter
fi

t=$(mktemp -d)
if [[ "$(uname)" =~ "MINGW" ]] || [[ "$(uname)" =~ "MSYS" ]] ; then
    PWDARG=-W
    t=$(cd "$t" && pwd $PWDARG)
else
    PWDARG=
fi
echo Temp dir: $t
export ASEPRITE_USER_FOLDER=$t

if [[ "$filter" == "" ]] || [[ "console" =~ $filter ]]; then
    echo ----------------------------------------------------------------------
    echo "Testing console..."
    echo "uname=$(uname)"

    $ASEPRITE -b --script scripts/console_assert.lua >$t/tmp 2>&1
    ! grep -q "this should be in the output" $t/tmp && fail "print() text not found in output"
    ! grep -q "assertion failed" $t/tmp && fail "assert() text not found in output"
    grep -q "this should not be in the output" $t/tmp && fail "text that shouldn't be in the output is"

    if [[ "$(uname)" =~ "MINGW" ]] || [[ "$(uname)" =~ "MSYS" ]] ; then
        echo Ignore console tests on Windows
    else
        $ASEPRITE -b --script scripts/console_print.lua >$t/tmp 2>&1
        echo -e "hello world\n1\t2\t3" >$t/tmp_expected
        ! diff -u $t/tmp $t/tmp_expected && fail
    fi
fi

first=0
result=0
for script in scripts/*.lua ; do
    [[ $script =~ console ]] && continue
    if [[ "$filter" != "" ]]; then
        [[ $script =~ $filter ]] || continue
    fi
    if [ $first == 0 ]; then
        echo ----------------------------------------------------------------------
        echo "Testing scripts..."
        first=1
    fi
    echo "Running $script"
    if ! $ASEPRITE -b --script $script >$t/tmp 2>&1 ; then
        echo FAILED && cat $t/tmp
        result=1
    fi
done

first=0
for script in cli/*.sh ; do
    if [[ "$filter" == "" ]] || [[ $script =~ $filter ]]; then
        if [ $first == 0 ]; then
            echo ----------------------------------------------------------------------
            echo "Testing CLI..."
            first=1
        fi
        echo "Running $script"
        source $script
    fi
done

echo ----------------------------------------------------------------------
if [ $result == 0 ] ; then
    echo Done
else
    echo FAILED
fi
echo ----------------------------------------------------------------------

exit $result
