# Copyright (C) 2001-2016  David Capello
# Find tests and add rules to compile them and run them

function(find_tests dir dependencies)
  file(GLOB tests ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/*_tests.cpp)
  list(REMOVE_AT ARGV 0)

  # Add gtest include directory so we can #include <gtest/gtest.h> in tests source code
  include_directories(${CMAKE_SOURCE_DIR}/third_party/gtest/include)

  # See if the test is linked with "laf-os" library.
  list(FIND dependencies laf-os link_with_os)
  if(link_with_os)
    set(extra_definitions -DLINKED_WITH_OS_LIBRARY)
  endif()

  foreach(testsourcefile ${tests})
    get_filename_component(testname ${testsourcefile} NAME_WE)

    add_executable(${testname} ${testsourcefile})
    add_test(NAME ${testname} COMMAND ${testname})

    if(MSVC)
      # Fix problem compiling gen from a Visual Studio solution
      set_target_properties(${testname}
        PROPERTIES LINK_FLAGS -ENTRY:"mainCRTStartup")
    endif()

    target_link_libraries(${testname} gtest ${ARGV} ${PLATFORM_LIBS})

    if(extra_definitions)
      set_target_properties(${testname}
        PROPERTIES COMPILE_FLAGS ${extra_definitions})
    endif()
  endforeach()
endfunction()
