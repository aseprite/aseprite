# Aseprite
# Copyright (C) 2019  Igara Studio S.A.
# Copyright (C) 2001-2017  David Capello

add_library(filters-lib
  brightness_contrast_filter.cpp
  color_curve.cpp
  color_curve_filter.cpp
  convolution_matrix.cpp
  convolution_matrix_filter.cpp
  filter.cpp
  hue_saturation_filter.cpp
  invert_color_filter.cpp
  median_filter.cpp
  outline_filter.cpp
  replace_color_filter.cpp)

target_link_libraries(filters-lib
  laf-base)
