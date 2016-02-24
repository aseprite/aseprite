# ASEPRITE
# Copyright (C) 2001-2013, 2016  David Capello

add_library(filters-lib
  color_curve.cpp
  color_curve_filter.cpp
  convolution_matrix.cpp
  convolution_matrix_filter.cpp
  invert_color_filter.cpp
  median_filter.cpp
  replace_color_filter.cpp)

target_link_libraries(filters-lib base-lib)
