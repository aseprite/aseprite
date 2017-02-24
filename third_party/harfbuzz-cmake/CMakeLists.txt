# Copyright (C) 2017 by David Capello

set(USE_UCDN 1)
set(USE_ICU 0)

add_definitions(-DHAVE_OT)

if(USE_UCDN)
  add_definitions(-DHAVE_UCDN)
  set(HARFBUZZ_EXTRA_SOURCES
    ../harfbuzz/src/hb-ucdn.cc)
endif()

if(USE_ICU)
  add_definitions(
    -DHAVE_ICU
    -DHAVE_ICU_BUILTIN)
  set(HARFBUZZ_EXTRA_SOURCES
    ../harfbuzz/src/hb-icu.cc)
endif()

add_library(harfbuzz
  ../harfbuzz/src/hb-blob.cc
  ../harfbuzz/src/hb-buffer-serialize.cc
  ../harfbuzz/src/hb-buffer.cc
  ../harfbuzz/src/hb-common.cc
  ../harfbuzz/src/hb-face.cc
  ../harfbuzz/src/hb-font.cc
  ../harfbuzz/src/hb-ft.cc
  ../harfbuzz/src/hb-ot-font.cc
  ../harfbuzz/src/hb-ot-layout.cc
  ../harfbuzz/src/hb-ot-map.cc
  ../harfbuzz/src/hb-ot-math.cc
  ../harfbuzz/src/hb-ot-shape-complex-arabic.cc
  ../harfbuzz/src/hb-ot-shape-complex-default.cc
  ../harfbuzz/src/hb-ot-shape-complex-hangul.cc
  ../harfbuzz/src/hb-ot-shape-complex-hebrew.cc
  ../harfbuzz/src/hb-ot-shape-complex-indic-table.cc
  ../harfbuzz/src/hb-ot-shape-complex-indic.cc
  ../harfbuzz/src/hb-ot-shape-complex-myanmar.cc
  ../harfbuzz/src/hb-ot-shape-complex-thai.cc
  ../harfbuzz/src/hb-ot-shape-complex-tibetan.cc
  ../harfbuzz/src/hb-ot-shape-complex-use-table.cc
  ../harfbuzz/src/hb-ot-shape-complex-use.cc
  ../harfbuzz/src/hb-ot-shape-fallback.cc
  ../harfbuzz/src/hb-ot-shape-normalize.cc
  ../harfbuzz/src/hb-ot-shape.cc
  ../harfbuzz/src/hb-ot-tag.cc
  ../harfbuzz/src/hb-ot-var.cc
  ../harfbuzz/src/hb-set.cc
  ../harfbuzz/src/hb-shape-plan.cc
  ../harfbuzz/src/hb-shape.cc
  ../harfbuzz/src/hb-shaper.cc
  ../harfbuzz/src/hb-unicode.cc
  ../harfbuzz/src/hb-warning.cc
  ${HARFBUZZ_EXTRA_SOURCES})

target_include_directories(harfbuzz PUBLIC
  ../harfbuzz/src)

if(USE_UCDN)
  add_library(ucdn
    ../harfbuzz/src/hb-ucdn/ucdn.c)
  target_include_directories(ucdn PUBLIC
    ../harfbuzz/src/hb-ucdn)
  target_link_libraries(harfbuzz ucdn)
endif(USE_UCDN)
