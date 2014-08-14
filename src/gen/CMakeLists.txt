# Aseprite Code Generator
# Copyright (C) 2014  David Capello

add_executable(gen
  gen.cpp
  ui_class.cpp)

target_link_libraries(gen base-lib)
if(USE_SHARED_TINYXML)
  target_link_libraries(gen ${LIBTINYXML_LIBRARY})
else()
  target_link_libraries(gen tinyxml)
endif()

