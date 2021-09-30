set(proj OpenIGTLinkIO)

# Set dependency list
set(${proj}_DEPENDS OpenIGTLink)
if(DEFINED Slicer_SOURCE_DIR)
  list(APPEND ${proj}_DEPENDS
    VTK
    )
endif()

# Include dependent projects if any
ExternalProject_Include_Dependencies(${proj} PROJECT_VAR proj DEPENDS_VAR ${proj}_DEPENDS)

if(${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  unset(OpenIGTLinkIO_DIR CACHE)
  find_package(OpenIGTLinkIO REQUIRED)
endif()

# Sanity checks
if(DEFINED OpenIGTLinkIO_DIR AND NOT EXISTS ${OpenIGTLinkIO_DIR})
  message(FATAL_ERROR "OpenIGTLinkIO_DIR variable is defined but corresponds to nonexistent directory")
endif()

if(NOT DEFINED OpenIGTLinkIO_DIR AND NOT ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})

  set(EP_SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj})
  set(EP_BINARY_DIR ${CMAKE_BINARY_DIR}/${proj}-build)

  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_${proj}_GIT_REPOSITORY
    "${EP_GIT_PROTOCOL}://github.com/IGSIO/OpenIGTLinkIO.git"
    QUIET
    )

  ExternalProject_SetIfNotDefined(
    ${CMAKE_PROJECT_NAME}_${proj}_GIT_TAG
    "master"
    QUIET
    )

  set(PYTHON_VARS)
  if (VTK_WRAP_PYTHON)
    list(APPEND PYTHON_VARS
    -DPYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE}
    -DPYTHON_LIBRARY:FILEPATH=${PYTHON_LIBRARY}
    -DPYTHON_INCLUDE_DIR:FILEPATH=${PYTHON_INCLUDE_DIR}
    -DPython3_EXECUTABLE:FILEPATH=${Python3_EXECUTABLE}
    -DPython3_LIBRARY:FILEPATH=${Python3_LIBRARY}
    -DPython3_INCLUDE_DIR:PATH=${Python3_INCLUDE_DIR}
    )
  endif()

  ExternalProject_Add(${proj}
    ${${proj}_EP_ARGS}
    GIT_REPOSITORY ${${CMAKE_PROJECT_NAME}_${proj}_GIT_REPOSITORY}
    GIT_TAG ${${CMAKE_PROJECT_NAME}_${proj}_GIT_TAG}
    SOURCE_DIR "${EP_SOURCE_DIR}"
    BINARY_DIR "${EP_BINARY_DIR}"
    CMAKE_CACHE_ARGS 
      ${ep_common_args}
      # Compiler settings
      -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
      -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
      -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
      -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
      -DCMAKE_CXX_STANDARD:STRING=${CMAKE_CXX_STANDARD}
      -DCMAKE_CXX_STANDARD_REQUIRED:BOOL=${CMAKE_CXX_STANDARD_REQUIRED}
      -DCMAKE_CXX_EXTENSIONS:BOOL=${CMAKE_CXX_EXTENSIONS}
      -DCMAKE_MACOSX_RPATH:BOOL=0
      # Output directories
      -DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=${CMAKE_BINARY_DIR}/${Slicer_THIRDPARTY_BIN_DIR}
      -DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH=${CMAKE_BINARY_DIR}/${Slicer_THIRDPARTY_LIB_DIR}
      -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY:PATH=${CMAKE_BINARY_DIR}/${Slicer_THIRDPARTY_LIB_DIR}
      -DOpenIGTLinkIO_BINARY_INSTALL:PATH=${Slicer_THIRDPARTY_BIN_DIR}
      -DOpenIGTLinkIO_LIBRARY_INSTALL:PATH=${Slicer_THIRDPARTY_LIB_DIR}
      -DOpenIGTLinkIO_ARCHIVE_INSTALL:PATH=${Slicer_THIRDPARTY_LIB_DIR}
      # Options
      -DBUILD_SHARED_LIBS:BOOL=ON
      -DBUILD_TESTING:BOOL=OFF
      -DIGTLIO_USE_GUI:BOOL=OFF
      # Dependencies
      -DOpenIGTLink_DIR:PATH=${OpenIGTLink_DIR}
      -DVTK_DIR:PATH=${VTK_DIR}
      ${PYTHON_VARS}
    INSTALL_COMMAND ""
    DEPENDS
      ${${proj}_DEPENDS}
    )  
  ExternalProject_GenerateProjectDescription_Step(${proj})

  set(OpenIGTLinkIO_DIR ${EP_BINARY_DIR})

  #-----------------------------------------------------------------------------
  # Launcher setting specific to build tree

  # library paths
  set(${proj}_LIBRARY_PATHS_LAUNCHER_BUILD
    ${OpenIGTLinkIO_DIR}
    ${OpenIGTLinkIO_DIR}/bin/<CMAKE_CFG_INTDIR>
    ${CMAKE_BINARY_DIR}/${Slicer_THIRDPARTY_BIN_DIR}
    ${CMAKE_BINARY_DIR}/${Slicer_THIRDPARTY_BIN_DIR}/<CMAKE_CFG_INTDIR>
    )
  mark_as_superbuild(
    VARS ${proj}_LIBRARY_PATHS_LAUNCHER_BUILD
    LABELS "LIBRARY_PATHS_LAUNCHER_BUILD"
    )

  #-----------------------------------------------------------------------------
  # Launcher setting specific to install tree

  # library paths
  set(${proj}_LIBRARY_PATHS_LAUNCHER_INSTALLED)
  if(UNIX  AND NOT APPLE)
    list(APPEND ${proj}_LIBRARY_PATHS_LAUNCHER_INSTALLED
      <APPLAUNCHER_DIR>/lib/igtl
      )
  endif()
  mark_as_superbuild(
    VARS ${proj}_LIBRARY_PATHS_LAUNCHER_INSTALLED
    LABELS "LIBRARY_PATHS_LAUNCHER_INSTALLED"
    )

else()
  ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDS})
endif()    

mark_as_superbuild(${proj}_DIR:PATH)
ExternalProject_Message(${proj} "${proj}_DIR:${${proj}_DIR}")
