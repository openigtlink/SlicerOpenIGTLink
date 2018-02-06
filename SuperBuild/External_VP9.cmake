set(proj VP9)

# Set dependency list
#set(${proj}_DEPENDENCIES "")

if(${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  unset(${proj}_DIR CACHE)
  include(${CMAKE_SOURCE_DIR}/SuperBuild/FindVP9.cmake)
endif()

# Sanity checks
if(DEFINED ${proj}_DIR AND NOT EXISTS ${${proj}_DIR})
  message(FATAL_ERROR "${proj}_DIR variable is defined but corresponds to nonexistent directory")
endif()

SET(proj_DEPENDENCIES)
IF(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows") # window os build doesn't need the yasm
  SET(YASM_BINARY_DIR "${CMAKE_BINARY_DIR}/Deps/VP9")
  INCLUDE(${CMAKE_SOURCE_DIR}/SuperBuild/External_yasm.cmake)
  IF(NOT YASM_FOUND)
    LIST(APPEND VP9_DEPENDENCIES YASM)
    message("VP9 dependencies modified." ${VP9_DEPENDENCIES})
  ENDIF()
ENDIF()

if(NOT DEFINED ${proj}_DIR AND NOT ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  # OpenIGTLink has not been built yet, so download and build it as an external project
  MESSAGE(STATUS "Downloading VP9 from https://github.com/webmproject/libvpx.git")  
  set(CMAKE_PROJECT_INCLUDE_EXTERNAL_PROJECT_ARG)            
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows") 
    SET (VP9_INCLUDE_DIR "${CMAKE_BINARY_DIR}/Deps/VP9/vpx" CACHE PATH "VP9 source directory" FORCE)
    SET (VP9_LIBRARY_DIR "${CMAKE_BINARY_DIR}/Deps/VP9" CACHE PATH "VP9 library directory" FORCE) 
    #file(MAKE_DIRECTORY ${VP9_LIBRARY_DIR})
    include(ExternalProjectForNonCMakeProject)

    # environment
    set(_env_script ${CMAKE_BINARY_DIR}/${proj}_Env.cmake)
    ExternalProject_Write_SetBuildEnv_Commands(${_env_script})

    set(_configure_cflags)
    #
    # To fix compilation problem: relocation R_X86_64_32 against `a local symbol' can not be
    # used when making a shared object; recompile with -fPIC
    # See http://www.cmake.org/pipermail/cmake/2007-May/014350.html
    #
    
    # configure step
    set(_configure_script ${CMAKE_BINARY_DIR}/${proj}_configure_step.cmake)
    file(WRITE ${_configure_script}
    "include(\"${_env_script}\")
    set(${proj}_WORKING_DIR \"${VP9_LIBRARY_DIR}\")
    ExternalProject_Execute(${proj} \"configure\" sh ${CMAKE_BINARY_DIR}/Deps/VP9/configure
      --disable-examples --as=yasm --enable-pic --disable-tools --disable-docs --disable-vp8 --disable-libyuv --disable-unit_tests --disable-postproc 
      WORKING_DIRECTORY ${proj}_WORKING_DIR
      )
    ")
    set(VP9_CONFIGURE_COMMAND ${CMAKE_COMMAND} -P ${_configure_script})

    # build step
    set(_build_script ${CMAKE_BINARY_DIR}/${proj}_build_step.cmake)
    file(WRITE ${_build_script}
    "include(\"${_env_script}\")
    set(${proj}_WORKING_DIR \"${VP9_LIBRARY_DIR}\")
    ExternalProject_Execute(${proj} \"build\" make WORKING_DIRECTORY ${proj}_WORKING_DIR)
    ")
    set(VP9_BUILD_COMMAND ${CMAKE_COMMAND} -P ${_build_script})

    # install step
    set(_install_script ${CMAKE_BINARY_DIR}/${proj}_install_step.cmake)
    file(WRITE ${_install_script}
    "include(\"${_env_script}\")
    set(${proj}_WORKING_DIR \"${VP9_LIBRARY_DIR}\")
    ExternalProject_Execute(${proj} \"install\" make install)
    ")
    set(VP9_INSTALL_COMMAND ${CMAKE_COMMAND} -P ${_install_script})

    ExternalProject_Add(${proj}
      GIT_REPOSITORY https://github.com/webmproject/libvpx/
      GIT_TAG v1.6.1
      UPDATE_COMMAND "" # Disable update
      SOURCE_DIR "${CMAKE_BINARY_DIR}/Deps/VP9"
      BUILD_IN_SOURCE ${VP9_BUILD_IN_SOURCE}
      CONFIGURE_COMMAND ${VP9_CONFIGURE_COMMAND}
      BUILD_COMMAND ${VP9_BUILD_COMMAND}
      INSTALL_COMMAND ${VP9_INSTALL_COMMAND}
      BUILD_ALWAYS 1
      DEPENDS
        ${${proj}_DEPENDENCIES}
      ) 
  else()
    if( ("${CMAKE_GENERATOR}" STREQUAL "Visual Studio 14 2015") OR ("${CMAKE_GENERATOR}" STREQUAL "Visual Studio 14 2015 Win64" ) OR 
        ("${CMAKE_GENERATOR}" STREQUAL "Visual Studio 12 2013") OR ("${CMAKE_GENERATOR}" STREQUAL "Visual Studio 12 2013 Win64" )
          )
      SET (VP9_INCLUDE_DIR "${CMAKE_BINARY_DIR}/Deps/VP9-Binary/vpx" CACHE PATH "VP9 source directory" FORCE)
      SET (BinaryURL "")
      if ("${CMAKE_GENERATOR}" STREQUAL "Visual Studio 14 2015") 
        SET (VP9_LIBRARY_DIR "${CMAKE_BINARY_DIR}/Deps/VP9-Binary/VP9-vs14" CACHE PATH "VP9 library directory" FORCE)
        SET (BinaryURL "https://github.com/openigtlink/CodecLibrariesFile/archive/vs14.zip")
      elseif("${CMAKE_GENERATOR}" STREQUAL "Visual Studio 14 2015 Win64" )  
        SET (VP9_LIBRARY_DIR "${CMAKE_BINARY_DIR}/Deps/VP9-Binary/VP9-vs14-Win64" CACHE PATH "VP9 library directory" FORCE) 
        SET (BinaryURL "https://github.com/openigtlink/CodecLibrariesFile/archive/vs14-Win64.zip")
      elseif ("${CMAKE_GENERATOR}" STREQUAL "Visual Studio 12 2013") 
        SET (VP9_LIBRARY_DIR "${CMAKE_BINARY_DIR}/Deps/VP9-Binary/VP9-vs12" CACHE PATH "VP9 library directory" FORCE)
        SET (BinaryURL "https://github.com/openigtlink/CodecLibrariesFile/archive/vs12.zip")
      elseif("${CMAKE_GENERATOR}" STREQUAL "Visual Studio 12 2013 Win64" )  
        SET (VP9_LIBRARY_DIR "${CMAKE_BINARY_DIR}/Deps/VP9-Binary/VP9-vs12-Win64" CACHE PATH "VP9 library directory" FORCE) 
        SET (BinaryURL "https://github.com/openigtlink/CodecLibrariesFile/archive/vs12-Win64.zip")  
      endif()  
      ExternalProject_Add(VP9
        URL               ${BinaryURL}
        SOURCE_DIR        "${CMAKE_BINARY_DIR}/Deps/VP9-Binary"
        CONFIGURE_COMMAND ""
        BUILD_ALWAYS 0
        BUILD_COMMAND ""
        INSTALL_COMMAND   ""
        TEST_COMMAND      ""
      )
    else()
      message(WARNING "Only support for Visual Studio 14 2015")
    endif()
  endif()
  if(APPLE)
    ExternalProject_Add_Step(${proj} vp9_install_chmod_library
      COMMAND chmod u+xw ${VP9_LIBRARY_DIR}/libvpx.a
      DEPENDEES install
      )
  endif()

  if(WIN32)
    if("${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
      SET(VP9_LIBRARY optimized ${VP9_LIBRARY_DIR}\\x64\\Release\\vpxmd.lib debug ${VP9_LIBRARY_DIR}\\x64\\Debug\\vpxmdd.lib)
    else()
      SET(VP9_LIBRARY optimized ${VP9_LIBRARY_DIR}\\Win32\\Release\\vpxmd.lib debug ${VP9_LIBRARY_DIR}\\Win32\\Debug\\vpxmdd.lib)
    endif()
  else()
    set(VP9_LIBRARY ${VP9_LIBRARY_DIR}/libvpx.a})
    set(${proj}_LIBRARY_PATHS_LAUNCHER_BUILD ${VP9_LIBRARY_DIR})
  endif()

  set(Slicer_VP9_DIR ${VP9_LIBRARY_DIR})

  #-----------------------------------------------------------------------------
  # Launcher setting specific to build tree

  # library paths
  mark_as_superbuild(
    VARS ${proj}_LIBRARY_PATHS_LAUNCHER_BUILD
    LABELS "LIBRARY_PATHS_LAUNCHER_BUILD"
    )

  # paths
  set(${proj}_PATHS_LAUNCHER_BUILD ${Slicer_VP9_DIR})
  mark_as_superbuild(
    VARS ${proj}_PATHS_LAUNCHER_BUILD
    LABELS "PATHS_LAUNCHER_BUILD"
    )

  # environment variables
  set(${proj}_ENVVARS_LAUNCHER_BUILD
    "VP9_LIBRARY=${Slicer_VP9_DIR}/vpx"
    )
  mark_as_superbuild(
    VARS ${proj}_ENVVARS_LAUNCHER_BUILD
    LABELS "ENVVARS_LAUNCHER_BUILD"
    )

  #-----------------------------------------------------------------------------
  # Launcher setting specific to install tree

  set(vp9lib_subdir lib)
  if(WIN32)
    set(vp9lib_subdir bin)
  endif()

  # library paths
  set(${proj}_LIBRARY_PATHS_LAUNCHER_INSTALLED <APPLAUNCHER_DIR>/lib/VP9/${vp9lib_subdir})
  mark_as_superbuild(
    VARS ${proj}_LIBRARY_PATHS_LAUNCHER_INSTALLED
    LABELS "LIBRARY_PATHS_LAUNCHER_INSTALLED"
    )

  # environment variables
  set(${proj}_ENVVARS_LAUNCHER_INSTALLED
    "VP9_LIBRARY=<APPLAUNCHER_DIR>/lib/VP9/lib/vp9"
    )
  mark_as_superbuild(
    VARS ${proj}_ENVVARS_LAUNCHER_INSTALLED
    LABELS "ENVVARS_LAUNCHER_INSTALLED"
    )
ENDIF()

set(VP9_DIR ${VP9_LIBRARY_DIR})
ExternalProject_Message(${proj} "VP9_DIR:${VP9_LIBRARY_DIR}")
mark_as_superbuild(VP9_DIR:PATH)
