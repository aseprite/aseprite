#! /usr/bin/env bash
set -euo pipefail

echo "======================= BUILD ASEPRITE HELPER ========================"

pwd="$(pwd)"

if [[ ! -f "$pwd/EULA.txt" || ! -f "$pwd/.gitmodules" ]]; then
  echo "Run build script from the Aseprite directory"
  exit 1
fi

if [[ "${1:-}" == "--reset" ]]; then
  echo "Resetting $pwd/.build directory"
  rm -f \
    "$pwd/.build/builds_dir" \
    "$pwd/.build/log" \
    "$pwd/.build/main_skia_dir" \
    "$pwd/.build/beta_skia_dir" \
    "$pwd/.build/userkind"
  rmdir "$pwd/.build" 2>/dev/null || true
  echo "Done"
  exit 0
fi

auto=
norun=

if [[ "${1:-}" == "--auto" ]]; then
  auto=1
  shift
fi

if [[ "${1:-}" == "--norun" ]]; then
  norun=1
  shift
fi

command -v cmake >/dev/null || { echo "cmake not found"; exit 1; }
command -v ninja >/dev/null || { echo "ninja not found"; exit 1; }

run_submodule_update=

while IFS= read -r module; do
  if [[ ! -f "$module/CMakeLists.txt" &&
        ! -f "$module/Makefile" &&
        ! -f "$module/makefile" &&
        ! -f "$module/Makefile.am" ]]; then
    if [[ $auto ]]; then
      run_submodule_update=1
      break
    else
      echo "Missing submodule: $module"
      echo "Run: git submodule update --init --recursive"
      exit 1
    fi
  fi
done < <(
  grep '^\[submodule' "$pwd/.gitmodules" | sed 's/.*"\(.*\)".*/\1/'
  grep '^\[submodule' "$pwd/laf/.gitmodules" | sed 's/.*"\(.*\)".*/laf\/\1/'
)

if [[ $run_submodule_update ]]; then
  git submodule update --init --recursive
fi

source "$pwd/laf/misc/platform.sh"

if [[ $is_win ]]; then
  command -v cl.exe >/dev/null || { echo "MSVC not found in PATH"; exit 1; }
fi

mkdir -p "$pwd/.build"

if [[ ! -f "$pwd/.build/userkind" ]]; then
  echo "user" > "$pwd/.build/userkind"
fi

userkind="$(cat "$pwd/.build/userkind")"

if [[ ! -f "$pwd/.build/builds_dir" ]]; then
  if [[ "$userkind" == "developer" && -n "${ASEPRITE_BUILD:-}" ]]; then
    builds_dir="$(cygpath "$ASEPRITE_BUILD" 2>/dev/null || echo "$ASEPRITE_BUILD")"
  else
    builds_dir="$pwd"
  fi
  mkdir -p "$builds_dir"
  echo "$builds_dir" > "$pwd/.build/builds_dir"
fi

builds_dir="$(cat "$pwd/.build/builds_dir")"

builds_list="$(mktemp)"

while IFS= read -r file; do
  if grep -q "CMAKE_PROJECT_NAME:STATIC=aseprite" "$file"; then
    echo "$file" >> "$builds_list"
  fi
done < <(ls "$builds_dir"/*/CMakeCache.txt 2>/dev/null | sort)

build_type=RelWithDebInfo
active_build_dir="$builds_dir/build"

if [[ -f "$active_build_dir/CMakeCache.txt" ]]; then
  source_dir="$(grep aseprite_SOURCE_DIR "$active_build_dir/CMakeCache.txt" | cut -d= -f2)"
else
  source_dir="$pwd"
fi

branch_name="$(git -C "$source_dir" rev-parse --abbrev-ref HEAD)"

skia_tag="$(cat "$pwd/laf/misc/skia-tag.txt")"
possible_skia_dir_name="skia-${skia_tag%%-*}"
file_skia_dir="${branch_name}_skia_dir"

if [[ ! -f "$pwd/.build/$file_skia_dir" ]]; then
  if [[ $is_win ]]; then
    skia_dir="C:/deps/$possible_skia_dir_name"
  else
    skia_dir="$HOME/deps/$possible_skia_dir_name"
  fi

  mkdir -p "$skia_dir"
  echo "$skia_dir" > "$pwd/.build/$file_skia_dir"
fi

skia_dir="$(cat "$pwd/.build/$file_skia_dir")"
mkdir -p "$skia_dir"

if [[ $is_win && "$build_type" == "Debug" ]]; then
  skia_library_dir="$skia_dir/out/Debug-x64"
else
  skia_library_dir="$skia_dir/out/Release-$cpu"
fi

if [[ ! -d "$skia_library_dir" ]]; then
  skia_build=Release
  [[ $is_win && "$build_type" == "Debug" ]] && skia_build=Debug

  skia_url="$(bash "$pwd/laf/misc/skia-url.sh" "$skia_build")"
  skia_file="$(basename "$skia_url")"

  command -v curl >/dev/null || { echo "curl required"; exit 1; }
  command -v unzip >/dev/null || { echo "unzip required"; exit 1; }

  curl -L -o "$skia_dir/$skia_file" "$skia_url"
  unzip -n -d "$skia_dir" "$skia_dir/$skia_file"
fi

cmake -B "$active_build_dir" -S "$source_dir" -G Ninja \
  -DCMAKE_BUILD_TYPE="$build_type" \
  -DLAF_BACKEND=skia \
  -DSKIA_DIR="$skia_dir" \
  -DSKIA_LIBRARY_DIR="$skia_library_dir"

cmake --build "$active_build_dir" -- aseprite

exe=""
[[ $is_win ]] && exe=".exe"

echo "Run:"
echo "  \"$active_build_dir/bin/aseprite$exe\""

if [[ $auto && ! $norun ]]; then
  "$active_build_dir/bin/aseprite$exe"
fi
