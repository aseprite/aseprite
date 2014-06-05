#! /bin/sh

echo -n "Where is Aseprite source code? "
read srcdir

echo -n "What version to release (e.g. 0.9.1-beta) ? "
read version

echo -n "What kind of distribution (e.g. production, trial) ? "
read distribution

destdir=$(pwd)/aseprite-release-$version

# --------------------------
# Clone the local repository
# --------------------------

if [ ! -d "$destdir" ] ; then
    git clone --depth=1 "$srcdir" "$destdir"
fi

# --------------
# Update version
# --------------

cd "$destdir/scripts"
sh update_version.sh $version

# ----------------------------------------------
# Make a build/ directory and compile with cmake
# ----------------------------------------------

if [ ! -d "$destdir/build" ] ; then
    mkdir "$destdir/build"
    cd "$destdir/build"
    if [ "$distribution" == "production" ]; then
        cmake \
            -D "CMAKE_BUILD_TYPE:STRING=RelWithDebInfo" \
            -D "CMAKE_CXX_FLAGS_RELWITHDEBINFO:STRING = /Zi /MT /O2 /Ob1 /D NDEBUG" \
            -D "CMAKE_C_FLAGS_RELWITHDEBINFO:STRING = /Zi /MT /O2 /Ob1 /D NDEBUG" \
            -D "CMAKE_EXE_LINKER_FLAGS:STRING = /MACHINE:X86 /SUBSYSTEM:WINDOWS,5.01" \
            -G "Ninja" \
            ..
    elif [ "$distribution" == "trial" ]; then
        cmake \
            -D "CMAKE_BUILD_TYPE:STRING=RelWithDebInfo" \
            -D "CMAKE_CXX_FLAGS_RELWITHDEBINFO:STRING = /Zi /MT /O2 /Ob1 /D NDEBUG" \
            -D "CMAKE_C_FLAGS_RELWITHDEBINFO:STRING = /Zi /MT /O2 /Ob1 /D NDEBUG" \
            -D "CMAKE_EXE_LINKER_FLAGS:STRING = /MACHINE:X86 /SUBSYSTEM:WINDOWS,5.01" \
            -D "ENABLE_TRIAL_MODE:BOOL=ON" \
            -G "Ninja" \
            ..
    fi
fi

# -------
# Compile
# -------

if [ ! -f "$destdir/aseprite.exe" ] ; then
    cd "$destdir/build"
    ninja aseprite

    cd "$destdir/build/src"
    signexe aseprite.exe
    cp aseprite.exe "$destdir"
fi

# ---------------
# Create packages
# ---------------

cd "$destdir/scripts"
sh create_packages.sh
