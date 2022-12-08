# Desktop Integration
# Copyright (c) 2022  Igara Studio S.A.

find_library(QUARTZ_LIBRARY Quartz)

add_library(AsepriteThumbnailer SHARED
  main.mm
  thumbnail.mm)

target_link_libraries(AsepriteThumbnailer
  laf-base
  dio-lib
  render-lib
  ${QUARTZ_LIBRARY})

set_target_properties(AsepriteThumbnailer PROPERTIES
  FRAMEWORK TRUE
  MACOSX_FRAMEWORK_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist")

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/lib/AsepriteThumbnailer.qlgenerator
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory lib/AsepriteThumbnailer.qlgenerator
  COMMAND ${CMAKE_COMMAND} -E make_directory lib/AsepriteThumbnailer.qlgenerator/Contents
  COMMAND ${CMAKE_COMMAND} -E make_directory lib/AsepriteThumbnailer.qlgenerator/Contents/MacOS
  COMMAND ${CMAKE_COMMAND} -E copy
             lib/AsepriteThumbnailer.framework/Versions/A/AsepriteThumbnailer
             lib/AsepriteThumbnailer.qlgenerator/Contents/MacOS
  COMMAND ${CMAKE_COMMAND} -E copy
             lib/AsepriteThumbnailer.framework/Versions/A/Resources/Info.plist
             lib/AsepriteThumbnailer.qlgenerator/Contents
  BYPRODUCTS ${CMAKE_BINARY_DIR}/lib/AsepriteThumbnailer.qlgenerator/Contents/MacOS/AsepriteThumbnailer
             ${CMAKE_BINARY_DIR}/lib/AsepriteThumbnailer.qlgenerator/Contents/Info.plist
  DEPENDS AsepriteThumbnailer)

add_custom_target(AsepriteThumbnailer.qlgenerator
  DEPENDS ${CMAKE_BINARY_DIR}/lib/AsepriteThumbnailer.qlgenerator)
