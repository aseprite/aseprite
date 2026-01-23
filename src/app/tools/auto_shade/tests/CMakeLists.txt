# Aseprite
# Copyright (C) 2026  Custom Build
#
# Auto-Shade Tool - Palette Generator Tests

# Function to add a palette generator test
function(add_palette_test name)
  add_executable(${name}
    ${name}.cpp
    ../palette_generator.cpp
  )
  add_test(NAME ${name} COMMAND ${name})
  target_include_directories(${name} PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/doc
    ${CMAKE_SOURCE_DIR}/laf
  )
  target_link_libraries(${name}
    doc-lib
    laf-gfx
  )
endfunction()

# Add the palette generator tests
add_palette_test(palette_generator_tests)
