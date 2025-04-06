#! /usr/bin/env bash
#
# Sets some environment variables to known the current platform.
# Usage:
#
# ./misc/platform.sh [--nocl]
#
# --nocl: For Windows only, it doesn't check the existence of cl.exe compiler in the PATH
#

uname="$(uname)"

if [[ "$uname" =~ "MINGW32" ]] || [[ "$uname" =~ "MINGW64" ]] || [[ "$uname" =~ "MSYS_NT-10.0" ]] ; then
    is_win=1
    cpu=x64

    if [[ "$1" != "--nocl" ]] && ! cl.exe >/dev/null 2>/dev/null ; then
        echo ""
        echo "MSVC compiler (cl.exe) not found in PATH"
        echo ""
        echo "  PATH=$PATH"
        echo ""
        exit 1
    fi
elif [[ "$uname" == "Linux" ]] ; then
    is_linux=1
    cpu=$(arch | xargs)
    if [[ "$cpu" == "x86_64" ]] ; then
        cpu=x64
    fi
elif [[ "$uname" =~ "Darwin" ]] ; then
    is_macos=1
    if [[ $(uname -m) == "arm64" ]]; then
        cpu=arm64
    else
        cpu=x64
    fi
fi
