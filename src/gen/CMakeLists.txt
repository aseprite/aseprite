# Aseprite Code Generator
# Copyright (C) 2014-2017  David Capello

add_executable(gen
  check_strings.cpp
  gen.cpp
  pref_types.cpp
  strings_class.cpp
  theme_class.cpp
  ui_class.cpp)

if(MSVC)
  # Fix problem compiling gen from a Visual Studio solution
  set_target_properties(gen
    PROPERTIES LINK_FLAGS -ENTRY:"mainCRTStartup")
endif()

target_link_libraries(gen
  laf-base
  cfg-lib
  tinyxml2::tinyxml2)
