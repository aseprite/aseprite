FROM fedora:35 as base

ENV CC=/usr/bin/clang
ENV CXX=/usr/bin/clang++
ENV PATH="/tmp/depot_tools:${PATH}"

RUN dnf install -y clang cmake ninja-build libX11-devel libXcursor-devel libXi-devel mesa-libGL-devel \
  fontconfig-devel git wget unzip python2 && \
  dnf clean all

WORKDIR /tmp

RUN git clone --recursive https://github.com/aseprite/aseprite.git
RUN git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
RUN git clone -b aseprite-m96 https://github.com/aseprite/skia.git
RUN ln -s /usr/bin/python2 depot_tools/python && python --version

FROM base as build_skia

WORKDIR /tmp/skia

RUN python tools/git-sync-deps && \
  gn gen out/Release-x64 --args="is_debug=false is_official_build=true skia_use_system_expat=false skia_use_system_icu=false skia_use_system_libjpeg_turbo=false skia_use_system_libpng=false skia_use_system_libwebp=false skia_use_system_zlib=false skia_use_sfntly=false skia_use_freetype=true skia_use_harfbuzz=true skia_pdf_subset_harfbuzz=true skia_use_system_freetype2=false skia_use_system_harfbuzz=false" && \
  ninja -C out/Release-x64 skia modules

FROM build_skia as build_aseprite

WORKDIR /tmp/aseprite/build

RUN cmake \
  -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=/tmp/bin \
  -D_CMAKE_TOOLCHAIN_PREFIX=llvm- \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DLAF_BACKEND=skia  \
  -DSKIA_DIR=/tmp/skia/ \
  -DSKIA_LIBRARY_DIR=/tmp/skia/out/Release-x64 \
  -DSKIA_LIBRARY=/tmp/skia/out/Release-x64/libskia.a \
  -G Ninja \
  .. 

RUN ninja aseprite

# based on
# https://github.com/aseprite/skia#skia-on-linux
# https://github.com/aseprite/aseprite/blob/main/INSTALL.md
# https://stackoverflow.com/a/7032021
# https://stackoverflow.com/questions/12376897/gcc-linker-errors-on-fedora-undefined-reference#12376921
# https://github.com/aseprite/aseprite/issues/2083#issuecomment-968322129
