#! /bin/sh

if [ ! -f upload_packages.sh ]; then
    echo You must run upload_packages.sh from scripts/ directory
    exit 1
fi

version=$(cat ../config.h | grep VERSION | sed -e 's/.*\"\(.*\)\"/\1/g')
python=c:/Python27/python.exe

echo "Uploading $version..."

echo -n "Google Code username? "
read username

echo -n "Google Code password? "
read password

$python googlecode_upload.py \
    --summary="ASEPRITE $version Source Code without 3rd party code" \
    --project=aseprite \
    --user=$username \
    --password=$password \
    --labels="Type-Source,OpSys-Linux" \
    aseprite-$version.tar.xz

$python googlecode_upload.py \
    --summary="ASEPRITE $version Source Code" \
    --project=aseprite \
    --user=$username \
    --password=$password \
    --labels="Featured,OpSys-All,Type-Source" \
    aseprite-$version.zip

$python googlecode_upload.py \
    --summary="ASEPRITE $version for Windows" \
    --project=aseprite \
    --user=$username \
    --password=$password \
    --labels="Featured,OpSys-Windows,Type-Archive" \
    aseprite-$version-win32.zip
