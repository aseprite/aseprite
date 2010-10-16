# For OS X frameworks to work you must add headers to the target's sources.
function(add_our_library target)
    add_library(${target} ${ARGN})
    set_target_properties(${target}
        PROPERTIES
        DEBUG_POSTFIX -debug
        PROFILE_POSTFIX -profile
        )
endfunction(add_our_library)

function(set_our_framework_properties target nm)
    if(WANT_FRAMEWORKS)
        if(WANT_EMBED)
            set(install_name_dir "@executable_path/../Frameworks")
        else()
            set(install_name_dir "${FRAMEWORK_INSTALL_PREFIX}")
        endif(WANT_EMBED)
        set_target_properties(${target}
            PROPERTIES
            FRAMEWORK on
            OUTPUT_NAME ${nm}
            INSTALL_NAME_DIR "${install_name_dir}"
            )
    endif(WANT_FRAMEWORKS)
endfunction(set_our_framework_properties)

function(install_our_library target)
    install(TARGETS ${target}
            LIBRARY DESTINATION "lib${LIB_SUFFIX}"
            ARCHIVE DESTINATION "lib${LIB_SUFFIX}"
            FRAMEWORK DESTINATION "${FRAMEWORK_INSTALL_PREFIX}"
            RUNTIME DESTINATION "bin"
            # Doesn't work, see below.
            # PUBLIC_HEADER DESTINATION "include"
            )
endfunction(install_our_library)

# Unfortunately, CMake's PUBLIC_HEADER support doesn't install into nested
# directories well, otherwise we could rely on install(TARGETS) to install
# header files associated with the target.  Instead we use the install(FILES)
# to install headers.  We reuse the MACOSX_PACKAGE_LOCATION property,
# substituting the "Headers" prefix with "include".
function(install_our_headers)
    foreach(hdr ${ARGN})
        get_source_file_property(LOC ${hdr} MACOSX_PACKAGE_LOCATION)
        string(REGEX REPLACE "^Headers" "include" LOC ${LOC})
        install(FILES ${hdr} DESTINATION ${LOC})
    endforeach()
endfunction(install_our_headers)

function(add_our_executable nm)
    add_executable(${nm} ${ARGN})
    target_link_libraries(${nm} allegro)
    if(PSP)
        add_psp_executable(${nm})
    endif(PSP)
endfunction()

# Oh my. CMake really is bad for this - but I couldn't find a better
# way.
function(sanitize_cmake_link_flags return)
   SET(acc_libs)
   foreach(lib ${ARGN})
      # Watch out for -framework options (OS X)
      IF (NOT lib MATCHES "-framework.*|.*framework")
         # Remove absolute path.
         string(REGEX REPLACE "/.*/(.*)" "\\1" lib ${lib})

         # Remove .a/.so/.dylib.
         string(REGEX REPLACE "lib(.*)\\.(a|so|dylib)" "\\1" lib ${lib})

         # Remove -l prefix if it's there already.
         string(REGEX REPLACE "-l(.*)" "\\1" lib ${lib})

        set(acc_libs "${acc_libs} -l${lib}")
      ENDIF()
   endforeach(lib)
   set(${return} ${acc_libs} PARENT_SCOPE)
endfunction(sanitize_cmake_link_flags)

function(copy_files target)
    if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
        return()
    endif()
    foreach(file ${ARGN})
        # The "./" is NOT redundant as CMAKE_CFG_INTDIR may be "/".
        add_custom_command(
            OUTPUT  "./${CMAKE_CFG_INTDIR}/${file}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${file}"
            COMMAND "${CMAKE_COMMAND}" -E copy
                    "${CMAKE_CURRENT_SOURCE_DIR}/${file}"
                    "./${CMAKE_CFG_INTDIR}/${file}"
            )
    endforeach()
    add_custom_target(${target} ALL DEPENDS ${ARGN})
endfunction()

# vim: set sts=4 sw=4 et:
