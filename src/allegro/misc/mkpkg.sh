#!/bin/sh
#
#	Shell script to create a MacOS X package of the library.
#	The package will hold an end-user version of Allegro,
#	installing just the system wide library framework.
#	The created package is compatible with MacOS X 10.2 and newer.
#
#	Thanks to macosxhints.com for the hint!
#
#	Usage: mkpkg <allegro_dir>
#
#	Will create a package in the current directory; allegro_dir
#	must be a valid Allegro directory structure, holding the
#	previously compiled MacOS X dynamic library.
#

if [ ! $# -eq 1 ]; then
        echo "Usage: mkpkg.sh <allegro_dir>"
        exit 1
fi


echo "Checking version number"
	version=$(sed -n -e 's/shared_version = //p' $1/makefile.ver)
	major_minor_version=$(sed -n -e 's/shared_major_minor = //p' $1/makefile.ver)

libname=$1/lib/macosx/liballeg-${version}.dylib

if [ ! -f $libname ]; then
	echo "Cannot find valid dynamic library archive"
	exit 1
fi


if [ -d dstroot ]; then
	rm -fr dstroot
fi

basename=allegro-enduser-${version}


###########################
# Prepare package structure
###########################

echo "Setting up package structure"

framework=dstroot/Library/Frameworks/Allegro.framework
mkdir -p $framework
mkdir -p ${framework}/Versions/${version}/Resources
cp $libname ${framework}/Versions/${version}/allegro
(cd $framework && {
	(cd Versions; ln -s $version Current)
	ln -s Versions/Current/Resources Resources
	ln -s Versions/Current/allegro allegro
})

infofile=${framework}/Resources/Info.plist
cat > $infofile << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
        <key>CFBundleIdentifier</key>
        <string>com.allegro.lib</string>
        <key>CFBundleName</key>
        <string>allegro</string>
        <key>CFBundleShortVersionString</key>
        <string>Allegro ${version}</string>
</dict>
</plist>
EOF


##################
# Make the package
##################

echo "Creating package"

packagefile=${basename}.pkg
if [ -d $packagefile ]; then rm -fr $packagefile; fi
mkdir -p -m 0755 ${packagefile}/Contents/Resources

echo pmkrpkg1 > ${packagefile}/Contents/PkgInfo

infofile=${packagefile}/Contents/Info.plist
cat > $infofile << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleGetInfoString</key>
	<string>Allegro $version (EndUser)</string>
	<key>CFBundleName</key>
	<string>Allegro $version (EndUser)</string>
	<key>CFBundleShortVersionString</key>
	<string>${version}</string>
	<key>IFMajorVersion</key>
	<integer>0</integer>
	<key>IFMinorVersion</key>
	<integer>0</integer>
	<key>IFPkgFlagAllowBackRev</key>
	<false/>
	<key>IFPkgFlagAuthorizationAction</key>
	<string>RootAuthorization</string>
	<key>IFPkgFlagDefaultLocation</key>
	<string>/</string>
	<key>IFPkgFlagInstallFat</key>
	<false/>
	<key>IFPkgFlagIsRequired</key>
	<false/>
	<key>IFPkgFlagRelocatable</key>
	<false/>
	<key>IFPkgFlagRestartAction</key>
	<string>NoRestart</string>
	<key>IFPkgFlagRootVolumeOnly</key>
	<true/>
	<key>IFPkgFlagUpdateInstalledLanguages</key>
	<false/>
	<key>IFPkgFormatVersion</key>
	<real>0.10000000149011612</real>
</dict>
</plist>
EOF

descfile=${packagefile}/Contents/Resources/Description.plist
cat > $descfile << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>IFPkgDescriptionDeleteWarning</key>
	<string></string>
	<key>IFPkgDescriptionDescription</key>
	<string>Allegro $version end user package</string>
	<key>IFPkgDescriptionTitle</key>
	<string>Allegro $version (EndUser)</string>
	<key>IFPkgDescriptionVersion</key>
	<string>${version}</string>
</dict>
</plist>
EOF

gcc -o _makedoc $1/docs/src/makedoc/*.c
./_makedoc -rtf ${packagefile}/Contents/Resources/ReadMe.rtf $1/misc/pkgreadme._tx

bomfile=${packagefile}/Contents/Archive.bom
mkbom dstroot $bomfile

paxfile=${packagefile}/Contents/Archive.pax.gz
(cd dstroot && pax -x cpio -w -z . > ../${paxfile})

postflight=${packagefile}/Contents/Resources/postflight
cat > $postflight << EOF
#!/bin/sh
mkdir -p /usr/local/lib
if [ -f /usr/local/lib/liballeg-${version}.dylib ]; then
	rm -f /usr/local/lib/liballeg-${version}.dylib
fi
if [ -f /usr/local/lib/liballeg-${major_minor_version}.dylib ]; then
	rm -f /usr/local/lib/liballeg-${major_minor_version}.dylib
fi
if [ -f /usr/local/lib/liballeg-4.dylib ]; then
	rm -f /usr/local/lib/liballeg-4.dylib
fi
if [ -f /usr/local/lib/liballeg.dylib ]; then
	rm -f /usr/local/lib/liballeg.dylib
fi
ln -s /Library/Frameworks/Allegro.framework/Versions/${version}/Allegro /usr/local/lib/liballeg-${version}.dylib
ln -s /Library/Frameworks/Allegro.framework/Versions/${version}/Allegro /usr/local/lib/liballeg-${major_minor_version}.dylib
ln -s /Library/Frameworks/Allegro.framework/Versions/${version}/Allegro /usr/local/lib/liballeg-4.dylib
ln -s /Library/Frameworks/Allegro.framework/Versions/${version}/Allegro /usr/local/lib/liballeg.dylib

EOF
chmod a+x $postflight

sizesfile=${packagefile}/Contents/Resources/Archive.sizes
numfiles=`lsbom -s $bomfile | wc -l`
size=`du -k dstroot | tail -n 1 | awk '{ print $1;}'`
zippedsize=`du -k ${packagefile} | tail -n 1 | awk '{ print $1;}'`
bomsize=`ls -l $bomfile | awk '{ print $5;}'`
infosize=`ls -l $infofile | awk '{ print $5;}'`
((size += (bomsize + infosize)))
echo NumFiles $numfiles > $sizesfile
echo InstalledSize $size >> $sizesfile
echo CompressedSize $zippedsize >> $sizesfile


######################
# Build the disk image
######################

echo "Creating compressed disk image"
volume=allegro-enduser-${version}
diskimage=${volume}.dmg
tempimage=temp.dmg
mountpoint=temp_volume
rm -fr $diskimage $tempimage $mountpoint
hdiutil create $tempimage -megabytes 4 -layout NONE
drive=`hdid -nomount $tempimage`
newfs_hfs -v "$volume" $drive
mkdir $mountpoint
mount -t hfs $drive $mountpoint

cp -r $packagefile $mountpoint
./_makedoc -ascii ${mountpoint}/readme.txt $1/misc/pkgreadme._tx
./_makedoc -ascii ${mountpoint}/CHANGES $1/docs/src/changes._tx
./_makedoc -part -ascii ${mountpoint}/AUTHORS $1/docs/src/thanks._tx
./_makedoc -part -ascii ${mountpoint}/THANKS $1/docs/src/thanks._tx

umount $mountpoint
hdiutil eject $drive
hdiutil convert -format UDCO $tempimage -o $diskimage
echo "Compressing image"
gzip -f -9 $diskimage
rm -fr $tempimage ${packagefile} dstroot _makedoc $mountpoint

echo "Done!"
