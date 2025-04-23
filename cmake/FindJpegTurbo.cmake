# Get libjpeg-turbo package
# Copyright (c) 2024  Igara Studio S.A.
#
# This file is released under the terms of the MIT license.
# Read LICENSE.txt for more information.
#
# Makes the libjpeg-turbo target available.
#

if(LAF_BACKEND STREQUAL "skia")

  find_library(LIBJPEG_TURBO_LIBRARY NAMES libjpeg jpeg
    HINTS "${SKIA_LIBRARY_DIR}" NO_DEFAULT_PATH)
  set(LIBJPEG_TURBO_INCLUDE_DIRS "${SKIA_DIR}/third_party/externals/libjpeg-turbo")

  add_library(libjpeg-turbo STATIC IMPORTED)
  set_target_properties(libjpeg-turbo PROPERTIES
    IMPORTED_LOCATION "${LIBJPEG_TURBO_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES ${LIBJPEG_TURBO_INCLUDE_DIRS})

else()

  if(WIN32)
    set(LIBJPEG_TURBO_STATIC_SUFFIX "-static")
  else()
    set(LIBJPEG_TURBO_STATIC_SUFFIX "")
  endif()

  include(ExternalProject)
  ExternalProject_Add(libjpeg-turbo-project
    URL https://github.com/libjpeg-turbo/libjpeg-turbo/archive/refs/tags/3.0.2.zip
    URL_HASH SHA512=c048c041f0bf205a8a3c8b8928d7a44299466253789f533db91f6ae4209a9074d5baef2fbb8e0a4215b4e3d2ba30c784f51b6c79ce0d2b1ea75440b8ffb23859
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/libjpeg-turbo"
    INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/libjpeg-turbo"
    BUILD_BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/libjpeg-turbo/lib/${CMAKE_STATIC_LIBRARY_PREFIX}jpeg${LIBJPEG_TURBO_STATIC_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}"
    CMAKE_CACHE_ARGS
    -DENABLE_SHARED:BOOL=OFF
    -DENABLE_STATIC:BOOL=ON
    -DWITH_ARITH_DEC:BOOL=ON
    -DWITH_ARITH_ENC:BOOL=ON
    -DWITH_JPEG8:BOOL=OFF
    -DWITH_JPEG7:BOOL=OFF
    -DWITH_TURBOJPEG:BOOL=OFF
    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    -DCMAKE_INSTALL_LIBDIR:PATH=<INSTALL_DIR>/lib)

  ExternalProject_Get_Property(libjpeg-turbo-project install_dir)
  set(LIBJPEG_TURBO_INCLUDE_DIRS "${install_dir}/include")
  set(LIBJPEG_TURBO_LIBRARY "${install_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}jpeg${LIBJPEG_TURBO_STATIC_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")

  # Create the directory so changing INTERFACE_INCLUDE_DIRECTORIES doesn't fail
  file(MAKE_DIRECTORY ${LIBJPEG_TURBO_INCLUDE_DIRS})

  add_library(libjpeg-turbo STATIC IMPORTED)
  set_target_properties(libjpeg-turbo PROPERTIES
    IMPORTED_LOCATION ${LIBJPEG_TURBO_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES ${LIBJPEG_TURBO_INCLUDE_DIRS})
  add_dependencies(libjpeg-turbo libjpeg-turbo-project)

endif()
