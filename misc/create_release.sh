#! /bin/sh

if [ ! -d aseprite ] ; then
    echo "You've to execute this file in the parent directory"
    echo "of your aseprite repository. Also, your aseprite clone"
    echo "should be in a directory called aseprite/"
    exit 1
fi

echo -n "What version to release (e.g. 0.9.1-beta) ? "
read version

destdir=aseprite-release-$version

# --------------------------
# Clone the local repository
# --------------------------

if [ ! -d $destdir ] ; then
    git clone --depth=1 aseprite $destdir
fi

# ----------------------------
# Copy the quick reference PDF
# ----------------------------

if [ ! -f $destdir/docs ] ; then
    cp aseprite/docs/quickref.pdf $destdir/docs
fi

# --------------
# Update version
# --------------

cd $destdir
cat config.h \
        | sed -e "s/define VERSION.*/define VERSION \"$version\"/" \
        > tmp
mv tmp config.h

cat data/gui.xml \
        | sed -e "s/gui version=\".*/gui version=\"$version\">/" \
        > tmp
mv tmp data/gui.xml

# ----------------------------------------------
# Make a build/ directory and compile with cmake
# ----------------------------------------------

if [ ! -d build ] ; then
    mkdir build
    cd build
    cmake \
        -D "CMAKE_BUILD_TYPE:STRING=RelWithDebInfo" \
        -D "CMAKE_CXX_FLAGS_RELWITHDEBINFO:STRING = /Zi /MT /O2 /Ob1 /D NDEBUG" \
        -D "CMAKE_C_FLAGS_RELWITHDEBINFO:STRING = /Zi /MT /O2 /Ob1 /D NDEBUG" \
        -G "NMake Makefiles" \
        ..
    cd ..
fi

# -------
# Compile
# -------

if [ ! -f aseprite.exe ] ; then
    cd build
    nmake
    cp src/aseprite.exe ..
    cd ..
fi

# ---------------
# Create packages
# ---------------

cd misc
sh dist.sh
