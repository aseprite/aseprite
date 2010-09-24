# ASE - Allegro Sprite Editor
# Copyright (C) 2001-2010  David Capello

include_directories(include ../../src)

add_library(vaca
  src/Application.cpp
  src/Component.cpp
  src/Event.cpp
  src/Exception.cpp
  src/Point.cpp
  src/PreferredSizeEvent.cpp
  src/Property.cpp
  src/Rect.cpp
  src/Referenceable.cpp
  src/Size.cpp
  src/String.cpp
  src/Vaca.cpp)
