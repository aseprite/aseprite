#! /usr/bin/env bash
#
# Prints the URL to download the Skia version for the given parameters.
# Usage:
#
#   ./.skia-url.sh [release|debug] [windows|macos|linux] [x86|x64|arm64]
#
# The first version will print the URL for the release version of the
# current platform.
#

script_dir=$(dirname "${BASH_SOURCE[0]}")
skia_tag=$(cat "$script_dir/skia-tag.txt" | xargs)
skia_build=Release

source "$script_dir/platform.sh" --nocl

while [[ "$1" != "" ]] ; do
    arg=$(echo $1 | tr '[:upper:]' '[:lower:]')
    if [[ "$arg" == "x64" ]] ; then
        cpu=x64
    elif [[ "$arg" == "x86" ]] ; then
        cpu=x86
    elif [[ "$arg" == "arm64" ]] ; then
        cpu=arm64
    elif [[ "$arg" == "win" || "$arg" == "windows" ]] ; then
        is_win=1
        is_macos=
        is_linux=
    elif [[ "$arg" == "macos" || "$arg" == "macosx" || "$arg" == "osx" ]] ; then
        is_win=
        is_macos=1
        is_linux=
    elif [[ "$arg" == "linux" || "$arg" == "unix" ]] ; then
        is_win=
        is_macos=
        is_linux=1
    elif [[ "$arg" == "debug" ]] ; then
        skia_build=Debug
    fi
    shift
done

if [ $is_win ] ; then
    skia_file=Skia-Windows-$skia_build-$cpu.zip
elif [ $is_macos ] ; then
    skia_file=Skia-macOS-$skia_build-$cpu.zip
else
    skia_file=Skia-Linux-$skia_build-$cpu.zip
fi

echo https://github.com/aseprite/skia/releases/download/$skia_tag/$skia_file
