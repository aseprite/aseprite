#! /bin/sh

dir="`pwd`"
version=$(cat ../config.h | grep VERSION | sed -e 's/.*\"\(.*\)\"/\1/g')
distdir=ASEPRITE_$version

if [ ! -f create_dmg.sh ]; then
    echo You must run create_dmg.sh from scripts/ directory
    exit 1
fi

if [ ! -f $distdir.dmg ] ; then
    cd "$dir/.."
    mkdir "$dir/$distdir"
    mkdir "$dir/$distdir/aseprite.app"
    mkdir "$dir/$distdir/aseprite.app/Contents"
    mkdir "$dir/$distdir/aseprite.app/Contents/MacOS"
    mkdir "$dir/$distdir/aseprite.app/Contents/Resources"
    cp -R LICENSE.txt docs "$dir/$distdir"
    cp -R aseprite "$dir/$distdir/aseprite.app/Contents/MacOS"
    cp -R "$dir/macosx/Info.plist" "$dir/$distdir/aseprite.app/Contents"
    cp -R data "$dir/macosx/aseprite.icns" "$dir/$distdir/aseprite.app/Contents/Resources"
    cd "$dir"

    hdiutil create "$distdir.dmg" -srcfolder "$distdir"
    rm -fr $distdir
fi
