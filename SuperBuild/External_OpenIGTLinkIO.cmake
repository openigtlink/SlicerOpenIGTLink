set(proj OpenIGTLinkIO)

# Set dependency list
set(${proj}_DEPENDENCIES "")

if(${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  unset(OpenIGTLinkIO_DIR CACHE)
  find_package(OpenIGTLinkIO REQUIRED NO_MODULE)
endif()

# Sanity checks
if(DEFINED OpenIGTLinkIO_DIR AND NOT EXISTS ${OpenIGTLinkIO_DIR})
  message(FATAL_ERROR "OpenIGTLinkIO_DIR variable is defined but corresponds to nonexistent directory")
endif()

SET (OpenIGTLinkIO_SRC_DIR "${CMAKE_BINARY_DIR}/Deps/OpenIGTLinkIO")
SET (Slicer_OpenIGTLinkIO_DIR "${CMAKE_BINARY_DIR}/Deps/OpenIGTLinkIO-bin" CACHE INTERNAL "Path to store OpenIGTLinkIO binaries")
if(NOT DEFINED OpenIGTLinkIO_DIR AND NOT ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  # OpenIGTLinkIO has not been built yet, so download and build it as an external project

  
  ExternalProject_Add( OpenIGTLinkIO
    PREFIX "${CMAKE_BINARY_DIR}/Deps/OpenIGTLinkIO-prefix"
    SOURCE_DIR "${OpenIGTLinkIO_SRC_DIR}"
    BINARY_DIR "${Slicer_OpenIGTLinkIO_DIR}"
    #--Download step--------------
    GIT_REPOSITORY https://github.com/IGSIO/OpenIGTLinkIO.git
    GIT_TAG master
    #--Configure step-------------
    CMAKE_ARGS 
      ${ep_common_args}
      -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY:PATH=${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
      -DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=${CMAKE_BINARY_DIR}/${Slicer_THIRDPARTY_BIN_DIR}
      -DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH=${CMAKE_BINARY_DIR}/${Slicer_THIRDPARTY_LIB_DIR}
      -DBUILD_SHARED_LIBS:BOOL=ON
      -DBUILD_EXAMPLES:BOOL=OFF
      -DBUILD_TESTING:BOOL=OFF
      -DVTK_DIR:PATH=${VTK_DIR}
      -DOpenIGTLink_DIR:PATH=${OpenIGTLink_DIR}
      -DIGTLIO_USE_GUI:BOOL=OFF
      -DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}
      -DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}
    #--Build step-----------------
    #--Install step-----------------
    INSTALL_COMMAND ""
    DEPENDS ${OpenIGTLinkIO_DEPENDENCIES}
    )  
  ExternalProject_GenerateProjectDescription_Step(${proj})

  set(OpenIGTLinkIO_DIR ${EP_BINARY_DIR})

  #-----------------------------------------------------------------------------
  # Launcher setting specific to build tree

  set(${proj}_LIBRARY_PATHS_LAUNCHER_BUILD
    ${OpenIGTLinkIO_DIR}
    ${OpenIGTLinkIO_DIR}/bin/<CMAKE_CFG_INTDIR>
    )

  #-----------------------------------------------------------------------------
  # Launcher setting specific to install tree

  if(UNIX AND NOT APPLE)
    set(${proj}_LIBRARY_PATHS_LAUNCHER_INSTALLED <APPLAUNCHER_DIR>/lib/igtl)
    mark_as_superbuild(
      VARS ${proj}_LIBRARY_PATHS_LAUNCHER_INSTALLED
      LABELS "LIBRARY_PATHS_LAUNCHER_INSTALLED"
      )
  endif()
ELSE()    
    # OpenIGTLinkIO has been built already
    FIND_PACKAGE(OpenIGTLinkIO REQUIRED PATHS ${OpenIGTLinkIO_DIR} NO_DEFAULT_PATH)
    MESSAGE(STATUS "Using OpenIGTLinkIO available at: ${OpenIGTLinkIO_DIR}")
    SET(Slicer_OpenIGTLinkIO_DIR "${OpenIGTLinkIO_DIR}")
ENDIF()    