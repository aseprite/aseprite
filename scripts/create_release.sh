#! /bin/sh

echo -n "Where is Aseprite source code? "
read srcdir

echo -n "What version to release (e.g. 0.9.1-beta) ? "
read version

echo -n "What kind of distribution (e.g. production, trial) ? "
read distribution

echo -n "What platform (e.g. win, mac) ? "
read platform

destdir=$(pwd)/aseprite-release-$version

if [ "$platform" == "win" ]; then
    exefile=aseprite.exe
    generator=ninja
elif [ "$platform" == "mac" ]; then
    exefile=aseprite
    generator="make -j4"
fi

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
sh $srcdir/scripts/update_version.sh $version

# ----------------------------------------------
# Make a build/ directory and compile with cmake
# ----------------------------------------------

if [ ! -d "$destdir/build" ] ; then
    mkdir "$destdir/build"
    cd "$destdir/build"

    if [ "$platform" == "win" ]; then
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
    elif [ "$platform" == "mac" ]; then
	if [ "$distribution" == "production" ]; then
            cmake \
		-D "CMAKE_BUILD_TYPE:STRING=RelWithDebInfo" \
		-G "Unix Makefiles" \
		..
	elif [ "$distribution" == "trial" ]; then
            cmake \
		-D "CMAKE_BUILD_TYPE:STRING=RelWithDebInfo" \
		-D "ENABLE_TRIAL_MODE:BOOL=ON" \
		-G "Unix Makefiles" \
		..
	fi
    fi
fi

# -------
# Compile
# -------

if [ ! -f "$destdir/$exefile" ] ; then
    cd "$destdir/build"
    $generator aseprite

    cd "$destdir/build/src"
    signexe $exefile
    cp $exefile "$destdir"
fi

# ---------------
# Create packages
# ---------------

cd "$destdir/scripts"
if [ "$platform" == "win" ]; then
    sh create_packages.sh
elif [ "$platform" == "mac" ]; then
    sh create_dmg.sh
fi
