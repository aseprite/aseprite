#! /bin/sh -e
#
#  Shell script to create a distribution zip, generating the manifest
#  file and optionally building diffs against a previous version. This
#  should be run from the allegro directory, but will leave the zip 
#  files in the parent directory.
#
#  This script tries to generate dependencies for all the supported
#  compilers, which involves considerable skulduggery to make sure it
#  will work on any Unix-like platform, even if the compilers are not
#  available themselves.
#
#  Note: if you write datestamp in the archive_name field, then the
#  resulting archive will be datestamped. This is in particular useful
#  for making SVN snapshots.


if [ $# -lt 1 -o $# -gt 2 ]; then
   echo "Usage: zipup archive_name [previous_archive]" 1>&2
   exit 1
fi

if [ -n "$2" ] && [ ! -r ../"$2" ]; then
   echo "Previous archive $2 not in parent directory, aborting"
   exit 1
fi


# This used to be necessary so that fixdll.sh would sort consistently.
# It probably isn't necessary any more.
export LC_ALL=en_US


# strip off the path and extension from our arguments
name=$(echo "$1" | sed -e 's/.*[\\\/]//; s/\.zip//')
prev=$(echo "$2" | sed -e 's/.*[\\\/]//; s/\.zip//')

# make a datestamped archive if specified
if test $name = datestamp; then
  date=`date '+%Y%m%d'`
  name=allegro_$date
fi

# make sure all the makefiles are in Unix text format
# for file in makefile.*; do
#    mv $file _tmpfile
#    tr -d \\\r < _tmpfile > $file
#    touch -r _tmpfile $file
#    rm _tmpfile
# done

# make sure all shell scripts are executable
find . -name '*.sh' -exec chmod +x {} ';'


proc_filelist()
{
   # common files.
   AL_FILELIST=`find . -type f "(" ! -path "*/.*" ")" -a "(" \
      -name "*.c" -o -name "*.cfg" -o -name "*.cpp" -o -name "*.def" -o \
      -name "*.h" -o -name "*.hin" -o -name "*.in" -o -name "*.inc" -o \
      -name "*.m" -o -name "*.m4" -o -name "*.mft" -o -name "*.s" -o \
      -name "*.rc" -o -name "*.rh" -o -name "*.spec" -o -name "*.pl" -o \
      -name "*.txt" -o -name "*._tx" -o -name "makefile*" -o \
      -name "*.inl" -o -name "configure" -o -name "CHANGES" -o \
      -name "AUTHORS" -o -name "THANKS" \
   ")"`

   # touch DOS batch files
  AL_FILELIST="$AL_FILELIST `find . -type f -name '*.bat'`"
}


# emulation of the djgpp utod utility program (Unix to DOS text format)
utod()
{
   for file in $*; do
      if echo $file | grep "^\.\.\./" >/dev/null; then
	 # files like .../*.c recurse into directories (emulating djgpp libc)
	 spec=$(echo $file | sed -e "s/^\.\.\.\///")
	 find . -type f -name "$spec" -exec perl -p -i -e 's/([^\r]|^)\n/\1\r\n/' {} \;
      else
	 perl -p -e "s/([^\r]|^)\n/\1\r\n/" $file > _tmpfile
	 touch -r $file _tmpfile
	 mv _tmpfile $file
      fi
   done
}


# convert documentation from the ._tx source files
echo "Converting documentation..."
TOP=$PWD
(
    set -e
    mkdir -p .dist/Build
    cd .dist/Build
    rm -rf docs
    cmake $TOP
    make -j2 docs
    umask 022
    mkdir -p $TOP/docs/{build,txt}
    cp -v docs/{AUTHORS,CHANGES,THANKS,readme.txt} $TOP
    cp -v docs/build/* $TOP/docs/build
    cp -v docs/txt/*.txt $TOP/docs/txt
) || exit 1


# generate docs for AllegroGL addon
echo "Generating AllegroGL docs..."
(cd addons/allegrogl/docs; doxygen >/dev/null)


# create language.dat and keyboard.dat files
misc/mkdata.sh || exit 1


# build the main zip archive
echo "Creating $name.zip..."
rm -f .dist/$name.zip
mkdir -p .dist/allegro
# It is recommended to build releases from a clean checkout.
# However, for testing it may be easier to use an existing workspace
# so let's ignore some junk here.
# Note: we use -print0 and xargs -0 to handle file names with spaces.
find . -type f \
	'!' -path "*/.*" -a \
	'!' -path '*/B*' -a \
	'!' -iname '*.rej' -a \
	'!' -iname '*.orig' -a \
	'!' -iname '*~' -a \
	-print0 |
    xargs -0 cp -a --parents -t .dist/allegro


# from now on, the scripts runs inside .dist
cd .dist


# generate the tar.gz first as the files are in Unix format
# This is missing the manifest file, but who cares?
VERSION=$(grep '^set(ALLEGRO_VERSION ' allegro/CMakeLists.txt | tr -cd '[0-9.]')
mv allegro allegro-$VERSION
tar zcf allegro-$VERSION.tar.gz allegro-$VERSION
mv allegro-$VERSION allegro


# convert files to DOS format for .zip and .7z archives
proc_filelist
utod "$AL_FILELIST"


# if 7za is available, use that to produce both .zip and .7z files
if 7za > /dev/null ; then
   7za a -mx9 $name.zip allegro
   7za a -mx9 -ms=on $name.7z allegro
else
   zip -9  -r $name.zip allegro
fi


# generate the manifest file
echo "Generating allegro.mft..."
unzip -Z1 $name.zip | sort > allegro/allegro.mft
echo "allegro/allegro.mft" >> allegro/allegro.mft
utod allegro/allegro.mft
zip -9 $name.zip allegro/allegro.mft


# if we are building diffs as well, do those
if [ $# -eq 2 ]; then

   echo "Inflating current version ($name.zip)..."
   mkdir current
   unzip -q $name.zip -d current

   echo "Inflating previous version ($2)..."
   mkdir previous
   unzip -q ../../"$2" -d previous || exit 1

   echo "Generating diffs..."
   set +e
   diff -U 3 -N --recursive previous/ current/ > $name.diff
   set -e

   echo "Deleting temp files..."
   rm -r previous
   rm -r current


   # generate the .txt file for the diff archive
   echo "This is a set of diffs to upgrade from $prev to $name." > $name.txt
   echo >> $name.txt
   echo "Date: $(date '+%A %B %d, %Y, %H:%M')" >> $name.txt
   echo >> $name.txt
   echo "To apply this patch, copy $name.diff into the same directory where you" >> $name.txt
   echo "installed Allegro (this should be one level up from the allegro/ dir, eg." >> $name.txt
   echo "if your Allegro installation lives in c:/djgpp/allegro/, you should be in" >> $name.txt
   echo "c:/djgpp/). Then type the command:" >> $name.txt
   echo >> $name.txt
   echo "   patch -p1 < $name.diff" >> $name.txt
   echo >> $name.txt
   echo "Change into the allegro directory, run make, and enjoy!" >> $name.txt

   utod $name.txt


   # zip up the diff archive
   echo "Creating ${name}_diff.zip..."
   if [ -f ${name}_diff.zip ]; then rm ${name}_diff.zip; fi
   zip -9 ${name}_diff.zip $name.diff $name.txt


   # find if we need to add any binary files as well
   bin=$(sed -n -e "s/Binary files previous\/\(.*\) and current.* differ/\1/p" $name.diff)

   if [ "$bin" != "" ]; then
      echo "Adding binary diffs..."
      zip -9 ${name}_diff.zip $bin
   fi

   rm $name.diff $name.txt

fi


echo "Done! Check .dist for the output."
