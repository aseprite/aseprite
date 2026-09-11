# Copyright (C) 2021  Igara Studio S.A.
# Copyright (C) 2001-2016  David Capello
# Find tests and add rules to compile them and run them

function(find_tests dir dependencies)
  file(GLOB tests ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/*_tests.cpp)
  list(REMOVE_AT ARGV 0)

  # See if the test is linked with "laf-os" library.
  list(FIND dependencies laf-os link_with_os)
  if(link_with_os)
    set(extra_definitions -DLINKED_WITH_OS_LIBRARY)
  endif()

  foreach(testsourcefile ${tests})
    get_filename_component(testname ${testsourcefile} NAME_WE)

    # Necessary for Extensions tests on Windows
    add_definitions(-DLIBARCHIVE_STATIC)

    add_executable(${testname} ${testsourcefile})
    add_test(NAME ${testname} COMMAND ${testname})

    if(MSVC)
      # Fix problem compiling gen from a Visual Studio solution
      set_target_properties(${testname}
        PROPERTIES LINK_FLAGS -ENTRY:"mainCRTStartup")
    endif()

    target_link_libraries(${testname} gtest ${ARGV} ${PLATFORM_LIBS})

    target_include_directories(${testname} PUBLIC
      # So we can include "tests/app_test.h"
      ${CMAKE_SOURCE_DIR}/src
      # Add gtest include directory so we can #include <gtest/gtest.h>
      # in tests source code
      ${CMAKE_SOURCE_DIR}/third_party/gtest/include)

    if(extra_definitions)
      set_target_properties(${testname}
        PROPERTIES COMPILE_FLAGS ${extra_definitions})
    endif()
  endforeach()
endfunction()
