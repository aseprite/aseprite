# Aseprite Code Generator
# Copyright (C) 2014  David Capello

add_executable(gen
  gen.cpp
  pref_types.cpp
  ui_class.cpp)

if(MSVC)
  # Fix problem compiling gen from a Visual Studio solution
  set_target_properties(gen
    PROPERTIES LINK_FLAGS -ENTRY:"mainCRTStartup")
endif()

target_link_libraries(gen base-lib)
if(USE_SHARED_TINYXML)
  target_link_libraries(gen ${LIBTINYXML_LIBRARY})
else()
  target_link_libraries(gen tinyxml)
endif()

