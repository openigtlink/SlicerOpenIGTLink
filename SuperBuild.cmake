
#-----------------------------------------------------------------------------
# Enable and setup External project global properties
#-----------------------------------------------------------------------------

set(ep_common_c_flags "${CMAKE_C_FLAGS_INIT} ${ADDITIONAL_C_FLAGS}")
set(ep_common_cxx_flags "${CMAKE_CXX_FLAGS_INIT} ${ADDITIONAL_CXX_FLAGS}")

#-----------------------------------------------------------------------------
# Project dependencies
#-----------------------------------------------------------------------------

include(ExternalProject)
if(Slicer_USE_VP9)
  INCLUDE(SuperBuild/External_VP9.cmake)
  SET(OpenIGTLink_DEPENDENCIES VP9)
endif()
INCLUDE(SuperBuild/External_OpenIGTLink.cmake)
SET(OpenIGTLinkIO_DEPENDENCIES OpenIGTLink)
INCLUDE(SuperBuild/External_OpenIGTLinkIO.cmake)
LIST(APPEND OpenIGTLinkIF_DEPENDENCIES OpenIGTLink)
LIST(APPEND OpenIGTLinkIF_DEPENDENCIES OpenIGTLinkIO)
foreach(dep ${EXTENSION_DEPENDS})
  mark_as_superbuild(${dep}_DIR)
endforeach()
set(projTop ${SUPERBUILD_TOPLEVEL_PROJECT})
ExternalProject_Include_Dependencies(${projTop}
  PROJECT_VAR projTop
  SUPERBUILD_VAR OpenIGTLinkIF_SUPERBUILD
  )

ExternalProject_Add(${projTop}
  ${${projTop}_EP_ARGS}
  DOWNLOAD_COMMAND ""
  INSTALL_COMMAND ""
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
  BINARY_DIR ${EXTENSION_BUILD_SUBDIRECTORY}
  CMAKE_CACHE_ARGS
    -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
    -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
    -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
    -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
    -DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    -DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY:PATH=${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
    -DOpenIGTLinkIF_SUPERBUILD:BOOL=OFF
    -DEXTENSION_SUPERBUILD_BINARY_DIR:PATH=${${EXTENSION_NAME}_BINARY_DIR}
    -DOpenIGTLinkIO_DIR:PATH=${Slicer_OpenIGTLinkIO_DIR}
    # Slicer configuration
    -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
    -DCMAKE_OSX_SYSROOT:PATH=${CMAKE_OSX_SYSROOT}
    -DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}
    -DSubversion_SVN_EXECUTABLE:PATH=${Subversion_SVN_EXECUTABLE}
    -DSlicer_DIR:PATH=${Slicer_DIR}
    -DSlicer_USE_VP9:BOOL=${Slicer_USE_VP9}
    -DVP9_LIBRARY:PATH=${VP9_LIBRARY}
    -DCMAKE_MACOSX_RPATH:BOOL=0
  DEPENDS
    ${OpenIGTLinkIF_DEPENDENCIES}
  )
