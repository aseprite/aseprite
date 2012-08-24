#! /bin/sh

if [ ! -f update_version.sh ]; then
    echo You must run update_version.sh from scripts/ directory
    exit 1
fi

version=$1
if [ "$version" == "" ]; then
    echo Usage: update_version.sh VERSION
    exit 1
fi

version_win32=$(echo $1 | sed -e 's/\./,/g' | sed -e 's/-.*$//')
commas=$(grep -o "," <<< "$version_win32" | wc -l)

while [ $commas -lt 3 ] ; do
    version_win32+=",0"
    commas=$(grep -o "," <<< "$version_win32" | wc -l)
done

sed -e "s/define VERSION.*/define VERSION \"$version\"/" < ../config.h > tmp
mv tmp ../config.h

sed -e "s/gui version=\".*/gui version=\"$version\">/" < ../data/gui.xml > tmp
mv tmp ../data/gui.xml

sed -e "s/FILEVERSION .*/FILEVERSION $version_win32/" < ../src/resources_win32.rc \
 | sed -e "s/PRODUCTVERSION .*/PRODUCTVERSION $version_win32/" \
 | sed -e "s/FileVersion\",.*/FileVersion\", \"$version_win32\"/" \
 | sed -e "s/ProductVersion\",.*/ProductVersion\", \"$version_win32\"/" \
 > tmp
mv tmp ../src/resources_win32.rc
