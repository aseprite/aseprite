# ASE - Allegro Sprite Editor
# Copyright (C) 2001-2010  David Capello

include_directories(include ../../src)

add_library(vaca
  src/Application.cpp
  src/Component.cpp
  src/Event.cpp
  src/Exception.cpp
  src/PreferredSizeEvent.cpp
  src/Property.cpp
  src/Referenceable.cpp
  src/String.cpp
  src/Vaca.cpp)
