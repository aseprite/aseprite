# Copyright (C) 2023  Igara Studio S.A.
# Find tinyxml2

function(find_tinyxml2 dir dependencies)
  file(GLOB tinyxml2 ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/tinyxml2.cpp)
  list(REMOVE_AT ARGV 0)

  get_filename_component(tinyxml2name tinyxml2.cpp NAME_WE)

  add_executable(${tinyxml2name} tinyxml2.cpp)

  if(MSVC)
    # Fix problem compiling gen from a Visual Studio solution
    set_target_properties(${tinyxml2name}
      PROPERTIES LINK_FLAGS -ENTRY:"mainCRTStartup")
  endif()

  target_link_libraries(${tinyxml2name} tinyxml2 ${ARGV} ${PLATFORM_LIBS})

  if(extra_definitions)
    set_target_properties(${tinyxml2name}
      PROPERTIES COMPILE_FLAGS ${extra_definitions})
  endif()
endfunction()
