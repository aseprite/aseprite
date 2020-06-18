# ASEPRITE
# Copyright (C) 2020  Igara Studio S.A.
# Copyright (C) 2001-2013  David Capello

add_library(tinyxml
  tinyxml.cpp
  tinyxmlerror.cpp
  tinyxmlparser.cpp)

# Use std::string instead of TiXmlString (we've found some threading
# issues related to TiXmlString::nullrep_)
target_compile_definitions(tinyxml PUBLIC TIXML_USE_STL)
