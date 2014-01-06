#! /bin/sh

echo -n "Where is Aseprite source code? "
read srcdir

echo -n "What version to release (e.g. 0.9.1-beta) ? "
read version

destdir=aseprite-release-$version

# --------------------------
# Clone the local repository
# --------------------------

if [ ! -d $destdir ] ; then
    git clone --depth=1 $srcdir $destdir
fi

# ----------------------------
# Copy the quick reference PDF
# ----------------------------

if [ ! -f $destdir/docs ] ; then
    cp $srcdir/docs/quickref.pdf $destdir/docs
fi

# --------------
# Update version
# --------------

cd $destdir/scripts
sh update_version.sh $version
cd ..

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

cd scripts
sh create_packages.sh
