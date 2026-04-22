# Desktop Integration
# Copyright (c) 2022-2025  Igara Studio S.A.

find_library(QUICKLOOK_THUMBNAILING_LIBRARY QuickLookThumbnailing)

add_executable(AsepriteThumbnailer MACOSX_BUNDLE
  thumbnail.mm)

target_link_libraries(AsepriteThumbnailer
  dio-lib
  render-lib
  ${QUICKLOOK_THUMBNAILING_LIBRARY})

set_target_properties(AsepriteThumbnailer PROPERTIES
  BUNDLE_EXTENSION appex
  LINK_FLAGS "-Wl,-e,_NSExtensionMain"
  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/Info.plist")
