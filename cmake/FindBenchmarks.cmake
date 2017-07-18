# Copyright (C) 2017  David Capello
# Find benchmarks

function(find_benchmarks dir dependencies)
  file(GLOB benchmarks ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/*_benchmark.cpp)
  list(REMOVE_AT ARGV 0)

  foreach(benchmarksourcefile ${benchmarks})
    get_filename_component(benchmarkname ${benchmarksourcefile} NAME_WE)

    add_executable(${benchmarkname} ${benchmarksourcefile})

    if(MSVC)
      # Fix problem compiling gen from a Visual Studio solution
      set_target_properties(${benchmarkname}
        PROPERTIES LINK_FLAGS -ENTRY:"mainCRTStartup")
    endif()

    target_link_libraries(${benchmarkname} benchmark ${ARGV} ${PLATFORM_LIBS})

    if(extra_definitions)
      set_target_properties(${benchmarkname}
        PROPERTIES COMPILE_FLAGS ${extra_definitions})
    endif()
  endforeach()
endfunction()
