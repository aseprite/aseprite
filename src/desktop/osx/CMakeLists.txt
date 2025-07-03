# Desktop Integration
# Copyright (c) 2022-2025  Igara Studio S.A.

find_library(QUICKLOOK_LIBRARY QuickLookThumbnailing)

add_library(AsepriteThumbnailer MODULE
  appex/thumbnails.mm)

target_link_libraries(AsepriteThumbnailer
  dio-lib
  render-lib
  ${QUICKLOOK_LIBRARY}
  )
