#! /bin/bash
#
# This is a script to help users and developers to build Aseprite.
# Usage:
#
#   ./build.sh
#   ./build.sh --reset
#   ./build.sh --auto [--norun]
#
# If you run this script without parameters, you will be able to
# follows the instructions and a configuration will be stored in a
# ".build" directory.
#
# Options:
#
#   --reset   Deletes the configuration and you can start over
#   --auto    Tries to build the default user configuration (release mode)
#   --norun   Doesn't auto-run when using --auto
#

echo "======================= BUILD ASEPRITE HELPER ========================"

# Check that we are running the script from the Aseprite clone directory.
pwd=$(pwd)
if [[ ! -f "$pwd/EULA.txt" || ! -f "$pwd/.gitmodules" ]] ; then
    echo ""
    echo "Run build script from the Aseprite directory"
    exit 1
fi

# Use "./build.sh --reset" to reset all the configuration (deletes
# .build directory).
if [ "$1" == "--reset" ] ; then
    echo ""
    echo "Resetting $pwd/.build directory"
    if [ -f "$pwd/.build/builds_dir" ] ; then rm $pwd/.build/builds_dir ; fi
    if [ -f "$pwd/.build/log" ] ; then rm $pwd/.build/log ; fi
    if [ -f "$pwd/.build/main_skia_dir" ] ; then rm $pwd/.build/main_skia_dir ; fi
    if [ -f "$pwd/.build/beta_skia_dir" ] ; then rm $pwd/.build/beta_skia_dir ; fi
    if [ -f "$pwd/.build/userkind" ] ; then rm $pwd/.build/userkind ; fi
    if [ -d "$pwd/.build" ] ; then rmdir $pwd/.build ; fi
    echo "Done"
    exit 0
fi

# Use "./build.sh --auto" to build the user configuration without
# questions (downloading Skia/release mode).
auto=
if [ "$1" == "--auto" ] ; then
    shift
    auto=1
fi
norun=
if [ "$1" == "--norun" ] ; then
    shift
    norun=1
fi

# Platform.
if [[ "$(uname)" =~ "MINGW32" ]] || [[ "$(uname)" =~ "MINGW64" ]] || [[ "$(uname)" =~ "MSYS_NT-10.0" ]] ; then
    is_win=1
    cpu=x64

    if ! cl.exe >/dev/null 2>/dev/null ; then
        echo ""
        echo "MSVC compiler (cl.exe) not found in PATH"
        echo ""
        echo "  PATH=$PATH"
        echo ""
        exit 1
    fi
elif [[ "$(uname)" == "Linux" ]] ; then
    is_linux=1
    cpu=x64
elif [[ "$(uname)" =~ "Darwin" ]] ; then
    is_macos=1
    if [[ $(uname -m) == "arm64" ]]; then
        cpu=arm64
    else
        cpu=x64
    fi
fi

# Check utilities.
if ! cmake --version >/dev/null ; then
    echo ""
    echo "cmake utility is not available. You can get cmake from:"
    echo ""
    echo "  https://cmake.org/download/"
    echo ""
    exit 1
fi
if ! ninja --version >/dev/null ; then
    echo ""
    echo "ninja utility is not available. You can get ninja from:"
    echo ""
    echo "  https://github.com/ninja-build/ninja/releases"
    echo ""
    exit 1
fi

# Check that all submodules are checked out.
run_submodule_update=
for module in $(cat "$pwd/.gitmodules" | \
                    grep '^\[submodule' | \
                    sed -e 's/^\[.*\"\(.*\)\"\]/\1/') \
              $(cat "$pwd/laf/.gitmodules" | \
                    grep '^\[submodule' | \
                    sed -e 's/^\[.*\"\(.*\)\"\]/laf\/\1/') ; do
    if [[ ! -f "$module/CMakeLists.txt" &&
          ! -f "$module/Makefile" &&
          ! -f "$module/makefile" &&
          ! -f "$module/Makefile.am" ]] ; then
        echo ""
        echo "Module $module doesn't exist."
        if [ $auto ] ; then
            run_submodule_update=1
            break
        else
            echo "Run:"
            echo ""
            echo "  git submodule update --init --recursive"
            echo ""
            exit 1
        fi
    fi
done
if [ $run_submodule_update ] ; then
    echo "Running:"
    echo ""
    echo "  git submodule update --init --recursive"
    echo ""
    if ! git submodule update --init --recursive ; then
        echo "Failed, try again"
        exit 1
    fi
    echo "Done"
fi

# Create the directory to store the configuration.
if [ ! -d "$pwd/.build" ] ; then
    mkdir "$pwd/.build"
fi

# Kind of user (User or Developer).
# For users we simplify the process (no multiple builds), for
# developers we have more options (debug mode, etc.).
if [ ! -f "$pwd/.build/userkind" ] ; then
    if [ $auto ] ; then
        echo "user" > $pwd/.build/userkind
    else
        echo ""
        echo "Select what kind of user you are (press U or D keys):"
        echo ""
        echo "  [U]ser: give a try to Aseprite"
        echo "  [D]eveloper: develop/modify Aseprite"
        echo ""
        read -sN 1 -p "[U/D]? "
        echo ""
        if [[ "$REPLY" == "d" || "$REPLY" == "D" ]] ; then
            echo "developer" > $pwd/.build/userkind
        elif [[ "$REPLY" == "u" || "$REPLY" == "U" ]] ; then
            echo "user" > $pwd/.build/userkind
        else
            echo "Use U or D keys to select kind of user/build process"
            exit 1
        fi
    fi
fi

userkind=$(echo -n $(cat $pwd/.build/userkind))
if [ "$userkind" == "developer" ] ; then
    echo "======================= BUILDING FOR DEVELOPER ======================="
else
    echo "========================= BUILDING FOR USER =========================="
fi

# Get the builds_dir location.
if [ ! -f "$pwd/.build/builds_dir" ] ; then
    if [ "$userkind" == "developer" ] ; then
        # The "builds" folder is a place where all possible combination/builds
        # will be stored. If the ASEPRITE_BUILD environment variable is
        # defined, that's the directory, in other case for a regular "user"
        # the folder will be this same directory where Aseprite was cloned.
        if [[ "$ASEPRITE_BUILD" != "" ]] ; then
            if [ $is_win ] ; then
                builds_dir=$(cygpath "$ASEPRITE_BUILD")
            else
                builds_dir="$ASEPRITE_BUILD"
            fi

            if [ -d "$builds_dir" ] ; then
                echo ""
                echo "Using ASEPRITE_BUILD environment variable for builds directory."
            else
                if ! mkdir "$builds_dir" ; then
                    echo ""
                    echo "The ASEPRITE_BUILD is defined but we weren't able to create the directory:"
                    echo ""
                    echo "  ASEPRITE_BUILD=$builds_dir"
                    echo ""
                    echo "To solve this issue delete the ASEPRITE_BUILD variable or point it to a valid path."
                    exit 1
                fi
            fi
        else
            # Default location for developers
            builds_dir=$HOME/builds

            echo ""
            echo "Select a folder where to leave all builds:"
            echo "  builds_dir/"
            echo "    release-x64/..."
            echo "    debug-x64/..."
            echo ""
            echo "builds_dir [$builds_dir]? "
            read builds_dir_read
            if [ "$builds_dir_read" != "" ] ; then
                builds_dir="$builds_dir_read"
            fi
        fi
    else
        # Default location for users
        builds_dir="$pwd"

        echo ""
        echo "We'll build Aseprite in $builds_dir/build directory"
    fi
    echo "$builds_dir" > "$pwd/.build/builds_dir"
fi
# Overwrite $builds_dir variable from the config content.
builds_dir="$(echo -n $(cat $pwd/.build/builds_dir))"

# List all builds.
builds_list="$(mktemp)"
n=1
for file in $(ls $builds_dir/*/CMakeCache.txt 2>/dev/null | sort) ; do
    if cat "$file" | grep -q "CMAKE_PROJECT_NAME:STATIC=aseprite" ; then
        if [ $n -eq 1 ] ; then
            echo "-- AVAILABLE BUILDS --"
        fi
        echo "$file" >> $builds_list
        echo "$n. $file"
        n=$(($n+1))
    fi
done

# New build configuration.
build_type=RelWithDebInfo

# No builds, so this is the first build.
if [[ $n -eq 1 ]] ; then
    echo "-- FIRST BUILD --"
    if [ "$userkind" == "developer" ] ; then
        active_build_dir="$builds_dir/aseprite-release"
    else
        active_build_dir="$builds_dir/build"
    fi
    echo "First build directory: $active_build_dir"
else
    echo "N. New build (N key)"
    echo "U. Update Visual Studio/Windows Kit/macOS SDK version (U key)"
    read -p "Select an option or number to build? " build_n

    # New build
    if [[ "$build_n" == "n" || "$build_n" == "N" ]] ; then
        read -p "Select build type [RELEASE/debug]? "
        if [[ "${REPLY,,}" == "debug" ]] ; then
            build_type=Debug
            new_build_name=aseprite-debug
        else
            build_type=RelWithDebInfo
            new_build_name=aseprite-release
        fi

        read -p "Select a name [$new_build_name]? "
        if [[ "$REPLY" != "" ]] ; then
            new_build_name=$REPLY
        fi
        active_build_dir="$builds_dir/$new_build_name"

    # Update SDK
    elif [[ "$build_n" == "u" || "$build_n" == "U" ]] ; then
        echo "Update SDK dirs..."
        if [ $is_win ] ; then
            newclver=$(echo $VCToolsInstallDir | sed -e 's_^.*\\\([0-9\.]*\)\\$_\1_')

            function update_file {
                file=$1
                echo "--- Updating $file ---" | tee -a "$pwd/.build/log"
                mv "$file" "$file-old"
                cat "$file-old" | sed -e 's_^\(.*/VC/Tools/MSVC/\)\([0-9\.]*\)\(.*$\)_\1'$newclver'\3_' > "$file"
                diff -w -u3 "$file-old" "$file" >> "$pwd/.build/log"
                echo "--- End $file ---" >> "$pwd/.build/log"
            }

            echo "New VC version: $newclver"
            for file in $(cat $builds_list) ; do
                build_dir=$(dirname $file)
                echo "--- Updating $build_dir ---"
                update_file "$file"
                for other_file in "$build_dir/CMakeFiles/rules.ninja" \
                                  "$build_dir"/CMakeFiles/*/*.cmake \
                                  "$build_dir/third_party/libpng/scripts/genout.cmake" ; do
                    update_file "$other_file"
                done
            done
        fi
        echo "Done"
        exit

    else # Build the selected dir
        n=1
        for file in $(cat $builds_list) ; do
            if [ "$n" == "$build_n" ] ; then
                active_build_dir=$(dirname $file)
                build_type=$(cat $active_build_dir/CMakeCache.txt | grep CMAKE_BUILD_TYPE | cut -d "=" -f2)
                break
            fi
            n=$(($n+1))
        done
    fi
fi

if [ "$active_build_dir" == "" ] ; then
    echo "No build selected"
    exit 1
fi

if [ -f "$active_build_dir/CMakeCache.txt" ] ; then
    source_dir=$(cat $active_build_dir/CMakeCache.txt | grep aseprite_SOURCE_DIR | cut -d "=" -f2)
else
    source_dir="$pwd"
fi
branch_name=$(git --git-dir="$source_dir/.git" rev-parse --abbrev-ref HEAD)

if [[ "$branch_name" == "main" || "$branch_name" == "beta" ]] ; then
    base_branch_name="$branch_name"
else
    # Get the origin (or first remote) name
    remote=$(git --git-dir="$source_dir/.git" remote | grep origin)
    if [ "$remote" == "" ] ; then
        remote=$(git --git-dir="$source_dir/.git" remote | head -n1)
    fi

    if git --git-dir="$source_dir/.git" branch --contains "$remote/beta" | grep -q "^\* $branch_name\$" ; then
        base_branch_name=beta
    elif git --git-dir="$source_dir/.git" branch --contains "$remote/main" | grep -q "^\* $branch_name\$" ; then
        base_branch_name=main
    else
        echo ""
        echo "Error: Branch $branch_name looks like doesn't belong to main or beta"
        echo ""
        exit 1
    fi
fi

echo "=========================== CONFIGURATION ============================"
echo "Build type: $build_type"
echo "Build dir: \"$active_build_dir\""
echo "Source dir: \"$source_dir\""
if [ "$branch_name" != "$base_branch_name" ] ; then
    echo "Branch name: $base_branch_name > $branch_name"
else
    echo "Branch name: $base_branch_name"
fi

# Required Skia for the base branch.
if [ "$base_branch_name" == "beta" ] ; then
    skia_tag=m124-08a5439a6b
    file_skia_dir=beta_skia_dir
    possible_skia_dir_name=skia-m124
else
    skia_tag=m102-861e4743af
    file_skia_dir=main_skia_dir
    possible_skia_dir_name=skia
fi

# Check Skia dependency.
if [ ! -f "$pwd/.build/$file_skia_dir" ] ; then
    # Try "C:/deps/skia" or "$HOME/deps/skia"
    if [[ $is_win ]] ; then
        skia_dir="C:/deps/$possible_skia_dir_name"
    else
        skia_dir="$HOME/deps/$possible_skia_dir_name"
    fi

    if [ ! -d "$skia_dir" ] ; then
        echo ""
        echo "Skia directory wasn't found."
        echo ""

        echo "Select Skia directory to create [$skia_dir]? "
        if [ ! $auto ] ; then
            read skia_dir_read
            if [ "$skia_dir_read" != "" ] ; then
                skia_dir="$skia_dir_read"
            fi
        fi
        mkdir -p $skia_dir || exit 1
    fi
    echo $skia_dir > "$pwd/.build/$file_skia_dir"
fi
skia_dir=$(echo -n $(cat $pwd/.build/$file_skia_dir))
if [ ! -d "$skia_dir" ] ; then
    mkdir "$skia_dir"
fi

# Only on Windows we need the Debug version of Skia to compile the
# Debug version of Aseprite.
if [[ $is_win && "$build_type" == "Debug" ]] ; then
    skia_library_dir="$skia_dir/out/Debug-x64"
else
    skia_library_dir="$skia_dir/out/Release-$cpu"
fi

# If the library directory is not available, we can try to download
# the pre-built package.
if [ ! -d "$skia_library_dir" ] ; then
    echo ""
    echo "Skia library wasn't found."
    echo ""
    if [ ! $auto ] ; then
        read -sN 1 -p "Download pre-compiled Skia automatically [Y/n]? "
    fi
    if [[ $auto || "$REPLY" == "" || "$REPLY" == "y" || "$REPLY" == "Y" ]] ; then
        if [[ $is_win && "$build_type" == "Debug" ]] ; then
            skia_build=Debug
        else
            skia_build=Release
        fi

        if [ $is_win ] ; then
            skia_file=Skia-Windows-$skia_build-$cpu.zip
        elif [ $is_macos ] ; then
            skia_file=Skia-macOS-$skia_build-$cpu.zip
        else
            skia_file=Skia-Linux-$skia_build-$cpu-libstdc++.zip
        fi
        skia_url=https://github.com/aseprite/skia/releases/download/$skia_tag/$skia_file
        if [ ! -f "$skia_dir/$skia_file" ] ; then
            curl -L -o "$skia_dir/$skia_file" "$skia_url"
        fi
        if [ ! -d "$skia_library_dir" ] ; then
            unzip -n -d "$skia_dir" "$skia_dir/$skia_file"
        fi
    else
        echo "Please follow these instructions to compile Skia manually:"
        echo ""
        echo "  https://github.com/aseprite/skia?tab=readme-ov-file#skia-for-aseprite-and-laf"
        echo ""
        exit 1
    fi
fi
echo "================================ SKIA ================================"
echo "Skia directory: \"$skia_dir\""
echo "Skia library directory: \"$skia_library_dir\""
if [ ! -d "$skia_library_dir" ] ; then
    echo "  But the Skia library directory wasn't found."
    exit 1
fi

# Building
echo "=============================== CMAKE ================================"
if [ ! -f "$active_build_dir/ninja.build" ] ; then
    echo ""
    echo "We are going to run cmake and then build the project."
    echo "This will take some minutes."
    echo ""
    if [ ! $auto ] ; then
        read -sN 1 -p "Press any key to continue. "
    fi

    if [ $is_macos ] ; then
        osx_deployment_target="-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0"
    else
        osx_deployment_target=
    fi

    echo "Configuring Aseprite..."
    if ! cmake -B "$active_build_dir" -S "$source_dir" -G Ninja \
         -DCMAKE_BUILD_TYPE=$build_type \
         $osx_deployment_target \
         -DLAF_BACKEND=skia \
         -DSKIA_DIR="$skia_dir" \
         -DSKIA_LIBRARY_DIR="$skia_library_dir" | \
            tee -a "$pwd/.build/log" ; then
        echo "Error running cmake."
        exit 1
    fi
fi
echo "============================== BUILDING =============================="
if ! cmake --build "$active_build_dir" -- aseprite | tee -a "$pwd/.build/log" ; then
    echo "Error building Aseprite."
    exit 1
fi

echo "================================ DONE ================================"
if [ $is_win ] ; then exe=.exe ; else exe= ; fi
echo "Run Aseprite with the following command:"
echo ""
echo "  $active_build_dir/bin/aseprite$exe"
echo ""

# Run Aseprite in --auto mode
if [[ $auto && ! $norun ]] ; then
    $active_build_dir/bin/aseprite$exe
fi
