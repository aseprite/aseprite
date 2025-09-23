# Desktop Integration
# Copyright (c) 2022-2025  Igara Studio S.A.

find_library(QUICKLOOK_LIBRARY QuickLookThumbnailing)

set(extension_target AsepriteThumbnailer)

add_executable(${extension_target} MACOSX_BUNDLE
  thumbnail.mm)

target_link_libraries(${extension_target}
  dio-lib
  render-lib
  ${QUICKLOOK_LIBRARY})

set_target_properties(${extension_target} PROPERTIES
  BUNDLE_EXTENSION appex
  LINK_FLAGS "-Wl,-e,_NSExtensionMain"
  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/Info.plist")
